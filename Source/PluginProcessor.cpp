#include "PluginProcessor.h"
#include "PluginEditor.h"

RONESidechainAudioProcessor::RONESidechainAudioProcessor()
 : AudioProcessor(BusesProperties().withInput("Bass", juce::AudioChannelSet::stereo(), true)
     .withInput("Kick", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
   state(*this, nullptr, "PARAMETERS", createParameters()) {
    for (auto& point : curve) point.store(1.0f, std::memory_order_relaxed);
    loadShape(0);
}

juce::AudioProcessorValueTreeState::ParameterLayout RONESidechainAudioProcessor::createParameters() {
    using P = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<juce::AudioParameterChoice>("division", "Cycle", juce::StringArray{"1/16","1/8","1/4","1/2","1 bar"}, 2));
    p.add(std::make_unique<juce::AudioParameterChoice>("shape", "Shape", juce::StringArray{"STEP","GATE","RAMP","PUMP","PLUCK","PULSE","SLOPE","SWELL","WAVE","RISE","NOTCH","BOUNCE","CUSTOM"}, 0));
    p.add(std::make_unique<P>("depth", "Depth", juce::NormalisableRange<float>(0,1), 1));
    p.add(std::make_unique<P>("click", "Click", juce::NormalisableRange<float>(0,10,0.1f), 2));
    p.add(std::make_unique<juce::AudioParameterChoice>("mode", "Duck", juce::StringArray{"FULL","SUB","BAND"}, 0));
    p.add(std::make_unique<P>("low", "Low", juce::NormalisableRange<float>(20,1000,1,0.35f), 80));
    p.add(std::make_unique<P>("high", "High", juce::NormalisableRange<float>(100,20000,1,0.35f), 5000));
    p.add(std::make_unique<juce::AudioParameterBool>("space", "SPACE", false));
    p.add(std::make_unique<P>("spaceAmount", "SPACE amount", juce::NormalisableRange<float>(0,1), .3f));
    p.add(std::make_unique<P>("spaceFrequency", "SPACE frequency", juce::NormalisableRange<float>(25,250,1,0.4f), 70));
    p.add(std::make_unique<P>("release", "Release", juce::NormalisableRange<float>(5,250,1), 29));
    p.add(std::make_unique<juce::AudioParameterBool>("phaseOn", "PHASE", false));
    p.add(std::make_unique<P>("split", "PHASE split", juce::NormalisableRange<float>(30,180,1,0.4f), 90));
    p.add(std::make_unique<P>("phase", "PHASE", juce::NormalisableRange<float>(-180,180,1), 0));
    return p;
}

void RONESidechainAudioProcessor::prepareToPlay(double sr, int block) {
    sampleRateHz = (float)sr;
    delaySamples = juce::jmax(1, (int)std::ceil(sampleRateHz * .010f));
    delayed.setSize(2, delaySamples + 1); delayed.clear(); delayWrite = 0; lowState.fill(0.0f);
    kickEnvelope = 0; spaceEnvelope = 0; kickAge = -(int)std::round(*state.getRawParameterValue("click") * .001f * sampleRateHz); kickCountdown = -1;
    lowState.fill(0.0f); highState.fill(0.0f); phaseX1.fill(0.0f); phaseY1.fill(0.0f);
    eqX1.fill(0.0f); eqX2.fill(0.0f); eqY1.fill(0.0f); eqY2.fill(0.0f);
    setLatencySamples(delaySamples);
}
bool RONESidechainAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainInputChannelSet() == juce::AudioChannelSet::mono() || l.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

float RONESidechainAudioProcessor::curveAt(float x) const noexcept {
    const float index = juce::jlimit(0.0f, 32.0f, x * 32.0f);
    const int i = (int)index;
    if (i >= 32) return curve[32].load(std::memory_order_relaxed);
    const float f = index - (float)i;
    return juce::jmap(f, curve[(size_t)i].load(std::memory_order_relaxed), curve[(size_t)i + 1].load(std::memory_order_relaxed));
}

void RONESidechainAudioProcessor::setCurvePoint(int i, float v) noexcept { if (i >= 0 && i < 33) curve[(size_t)i].store(juce::jlimit(0.0f, 1.0f, v), std::memory_order_relaxed); }
float RONESidechainAudioProcessor::getCurvePoint(int i) const noexcept { return (i >= 0 && i < 33) ? curve[(size_t)i].load(std::memory_order_relaxed) : 0.0f; }
void RONESidechainAudioProcessor::loadShape(int shape) noexcept {
    shape = juce::jlimit(0, 12, shape);
    if (shape == 12) return;
    for (int i=0;i<=32;++i) {
        const float x=(float)i/32.0f; float y=1.0f;
        switch(shape) {
            case 0: y = x < .2f ? 0.0f : 1.0f; break;
            case 1: y = x < .12f ? 0.0f : 1.0f; break;
            case 2: y = x; break;
            case 3: y = 1.0f-std::exp(-7.0f*x); break;
            case 4: y = std::pow(x, .42f); break;
            case 5: y = x < .5f ? .08f : .95f; break;
            case 6: y = .18f+.82f*x; break;
            case 7: y = std::sin(x*juce::MathConstants<float>::halfPi); break;
            case 8: y = .5f-.5f*std::cos(x*juce::MathConstants<float>::twoPi); break;
            case 9: y = std::sqrt(x); break;
            case 10: y = x < .3f ? 0.0f : (x < .58f ? 1.0f : .32f); break;
            case 11: y = juce::jlimit(0.0f,1.0f,1.25f*x + .18f*std::sin(x*juce::MathConstants<float>::twoPi)); break;
        }
        setCurvePoint(i,y);
    }
}

void RONESidechainAudioProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int bassN = getBusBuffer(b, true, 0).getNumChannels();
    auto bass = getBusBuffer(b, true, 0);
    auto kick = getBusBuffer(b, true, 1);
    auto out = getBusBuffer(b, false, 0);
    const int n = b.getNumSamples();
    const float depth = *state.getRawParameterValue("depth");
    const int clickN = juce::jlimit(0, delaySamples, (int)std::round(*state.getRawParameterValue("click") * .001f * sampleRateHz));
    const int division = (int)*state.getRawParameterValue("division");
    const double bpm = (getPlayHead() && getPlayHead()->getPosition() && getPlayHead()->getPosition()->getBpm()) ? *getPlayHead()->getPosition()->getBpm() : 120.0;
        const double beats = std::array<double,5>{.25,.5,1,2,4}[(size_t)division];
    const int cycle = juce::jmax(1, (int)(sampleRateHz * 60.0 * beats / bpm));
    const int mode = (int)*state.getRawParameterValue("mode");
    const float lowHz = *state.getRawParameterValue("low"), highHz = *state.getRawParameterValue("high");
    const float splitHz = *state.getRawParameterValue("split"), phaseDegrees = *state.getRawParameterValue("phase");
    const bool phaseEnabled = *state.getRawParameterValue("phaseOn") > .5f;
    const float spaceAmountValue = *state.getRawParameterValue("spaceAmount"), spaceFrequency = *state.getRawParameterValue("spaceFrequency");
    const float releaseMs = *state.getRawParameterValue("release");
    const float omega = juce::MathConstants<float>::twoPi * juce::jlimit(20.0f, 18000.0f, spaceFrequency) / sampleRateHz;
    const float alpha = std::sin(omega) / 16.0f;
    const float A = std::pow(10.0f, (-12.0f * spaceAmountValue) / 40.0f), a0 = 1.0f + alpha / A;
    eqB0=(1.0f+alpha*A)/a0; eqB1=(-2.0f*std::cos(omega))/a0; eqB2=(1.0f-alpha*A)/a0;
    eqA1=eqB1; eqA2=(1.0f-alpha/A)/a0;
    const float phaseOmega=juce::MathConstants<float>::twoPi*juce::jlimit(20.0f,18000.0f,splitHz)/sampleRateHz;
    const float q=std::tan(-phaseDegrees*juce::MathConstants<float>::pi/360.0f)/juce::jmax(.0001f,std::tan(phaseOmega*.5f));
    phaseA=juce::jlimit(-.999f,.999f,(1.0f-q)/(1.0f+q));
    float env = kickEnvelope;
    for (int i=0;i<n;++i) {
        float k=0;
        if (kick.getNumChannels()>0) for(int c=0;c<kick.getNumChannels();++c) k=juce::jmax(k,std::abs(kick.getReadPointer(c)[i]));
        env = juce::jmax(k, env * (k>env ? .4f : .997f));
        if (env > .12f && kickEnvelope <= .12f) kickCountdown = delaySamples - clickN;
        if (kickCountdown >= 0) {
            if (kickCountdown == 0) { kickAge = -clickN; kickCountdown = -1; }
            else --kickCountdown;
        }
        kickEnvelope = env;
        float gain;
        if (kickAge < 0) gain = clickN > 0 ? juce::jlimit(0.0f, 1.0f, -(float)kickAge / (float)clickN) : 1.0f;
        else gain = 1.0f - depth * (1.0f - curveAt((float)kickAge/cycle));
        if (kickAge == 0) gain = 0.0f;
        const float smooth = 1.0f - std::exp(-6.2831853f * juce::jlimit(20.0f, 18000.0f, lowHz) / sampleRateHz);
        const float highSmooth = 1.0f - std::exp(-6.2831853f * juce::jlimit(20.0f,18000.0f,highHz)/sampleRateHz);
        const float phaseSmooth = 1.0f - std::exp(-6.2831853f * juce::jlimit(20.0f,18000.0f,splitHz)/sampleRateHz);
        const float spaceTarget=(*state.getRawParameterValue("space")>.5f && env>.03f)?juce::jlimit(0.0f,1.0f,env*2.0f):0.0f;
        const float spaceCoeff=std::exp(-1.0f/((spaceTarget>spaceEnvelope?1.0f:releaseMs)*.001f*sampleRateHz));
        spaceEnvelope=spaceTarget+spaceCoeff*(spaceEnvelope-spaceTarget);
        for(int c=0;c<out.getNumChannels();++c) {
            const float x = bass.getReadPointer(juce::jmin(c,bassN-1))[i];
            delayed.setSample(c, delayWrite, x);
            const int read = (delayWrite + delayed.getNumSamples() - delaySamples) % delayed.getNumSamples();
            const float delayedX = delayed.getSample(c, read);
            const float lpLow=smooth*delayedX+(1.0f-smooth)*lowState[(size_t)c]; lowState[(size_t)c]=lpLow;
            const float lpHigh=highSmooth*delayedX+(1.0f-highSmooth)*highState[(size_t)c]; highState[(size_t)c]=lpHigh;
            float filtered=mode==1?lpLow:mode==2?lpHigh-lpLow:delayedX;
            if(phaseEnabled){const float sub=phaseSmooth*delayedX+(1.0f-phaseSmooth)*phaseX1[(size_t)c];const float rotated=phaseA*sub+phaseX1[(size_t)c]-phaseA*phaseY1[(size_t)c];phaseX1[(size_t)c]=sub;phaseY1[(size_t)c]=rotated;filtered=delayedX-sub+rotated;}
            float y=delayedX+filtered*(gain-1.0f);
            const float eq=eqB0*y+eqB1*eqX1[(size_t)c]+eqB2*eqX2[(size_t)c]-eqA1*eqY1[(size_t)c]-eqA2*eqY2[(size_t)c];
            eqX2[(size_t)c]=eqX1[(size_t)c]; eqX1[(size_t)c]=y; eqY2[(size_t)c]=eqY1[(size_t)c]; eqY1[(size_t)c]=eq;
            y+=(eq-y)*spaceEnvelope;
            out.getWritePointer(c)[i] = y;
        }
        if (out.getNumChannels() > 0) delayWrite = (delayWrite + 1) % delayed.getNumSamples();
        ++kickAge;
        if (kickAge >= cycle - clickN) kickAge = -clickN;
        juce::ignoreUnused(highHz);
    }
}
juce::AudioProcessorEditor* RONESidechainAudioProcessor::createEditor(){ return new RONESidechainAudioProcessorEditor(*this); }
void RONESidechainAudioProcessor::getStateInformation(juce::MemoryBlock& d){ auto x=state.copyState(); juce::ValueTree c("DRAWN_CURVE"); for(int i=0;i<=32;++i)c.setProperty("p"+juce::String(i),getCurvePoint(i),nullptr); x.addChild(c,-1,nullptr); std::unique_ptr<juce::XmlElement> xml(x.createXml()); copyXmlToBinary(*xml,d); }
void RONESidechainAudioProcessor::setStateInformation(const void* d,int n){ std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(d,n)); if(xml && xml->hasTagName(state.state.getType())) { auto x=juce::ValueTree::fromXml(*xml); auto c=x.getChildWithName("DRAWN_CURVE"); if(c.isValid()) for(int i=0;i<=32;++i) setCurvePoint(i,(float)c.getProperty("p"+juce::String(i),getCurvePoint(i))); x.removeChild(c,nullptr); state.replaceState(x); } }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){ return new RONESidechainAudioProcessor(); }
