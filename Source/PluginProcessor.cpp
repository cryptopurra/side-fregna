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
    p.add(std::make_unique<P>("release", "Release", juce::NormalisableRange<float>(5,250,1), 29));
    p.add(std::make_unique<P>("split", "PHASE split", juce::NormalisableRange<float>(30,180,1,0.4f), 90));
    p.add(std::make_unique<P>("phase", "PHASE", juce::NormalisableRange<float>(-180,180,1), 0));
    return p;
}

void RONESidechainAudioProcessor::prepareToPlay(double sr, int block) {
    sampleRateHz = (float)sr; delayed.setSize(2, block + 32); delayed.clear(); delayWrite = 0;
    kickEnvelope = 0; samplesSinceKick = 0;
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
    const float clickN = *state.getRawParameterValue("click") * .001f * sampleRateHz;
    const int division = (int)*state.getRawParameterValue("division");
    const double bpm = (getPlayHead() && getPlayHead()->getPosition() && getPlayHead()->getPosition()->getBpm()) ? *getPlayHead()->getPosition()->getBpm() : 120.0;
        const double beats = std::array<double,5>{.25,.5,1,2,4}[(size_t)division];
    const int cycle = juce::jmax(1, (int)(sampleRateHz * 60.0 * beats / bpm));
    const int mode = (int)*state.getRawParameterValue("mode");
    const float lowHz = *state.getRawParameterValue("low"), highHz = *state.getRawParameterValue("high");
    float env = kickEnvelope;
    for (int i=0;i<n;++i) {
        float k=0;
        if (kick.getNumChannels()>0) for(int c=0;c<kick.getNumChannels();++c) k=juce::jmax(k,std::abs(kick.getReadPointer(c)[i]));
        env = juce::jmax(k, env * (k>env ? .4f : .997f));
        if (env > .12f && kickEnvelope <= .12f) samplesSinceKick = 0;
        else ++samplesSinceKick;
        kickEnvelope = env;
        const int pos = samplesSinceKick % cycle;
        float gain = 1.0f - depth * (1.0f - curveAt((float)pos/cycle));
        if (samplesSinceKick == 0) gain = 0.0f;
        if (clickN > 0 && pos < clickN) gain *= (float)pos / clickN;
        const float smooth = 1.0f - std::exp(-6.2831853f * juce::jlimit(20.0f, 18000.0f, lowHz) / sampleRateHz);
        for(int c=0;c<out.getNumChannels();++c) {
            const float x = bass.getReadPointer(juce::jmin(c,bassN-1))[i];
            float filtered = x;
            if (mode == 1) { const float prev = i>0 ? bass.getReadPointer(juce::jmin(c,bassN-1))[i-1] : 0; filtered = smooth * x + (1-smooth)*prev; }
            else if (mode == 2) { const float prev = i>0 ? bass.getReadPointer(juce::jmin(c,bassN-1))[i-1] : 0; const float lp = smooth*x+(1-smooth)*prev; filtered = x-lp; }
            float y = x + (filtered * (gain - 1.0f));
            if (*state.getRawParameterValue("space") > .5f && env > .03f) y *= 1.0f - *state.getRawParameterValue("spaceAmount") * env;
            out.getWritePointer(c)[i] = y;
        }
        juce::ignoreUnused(highHz);
    }
}
juce::AudioProcessorEditor* RONESidechainAudioProcessor::createEditor(){ return new RONESidechainAudioProcessorEditor(*this); }
void RONESidechainAudioProcessor::getStateInformation(juce::MemoryBlock& d){ auto x=state.copyState(); juce::ValueTree c("DRAWN_CURVE"); for(int i=0;i<=32;++i)c.setProperty("p"+juce::String(i),getCurvePoint(i),nullptr); x.addChild(c,-1,nullptr); std::unique_ptr<juce::XmlElement> xml(x.createXml()); copyXmlToBinary(*xml,d); }
void RONESidechainAudioProcessor::setStateInformation(const void* d,int n){ std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(d,n)); if(xml && xml->hasTagName(state.state.getType())) { auto x=juce::ValueTree::fromXml(*xml); auto c=x.getChildWithName("DRAWN_CURVE"); if(c.isValid()) for(int i=0;i<=32;++i) setCurvePoint(i,(float)c.getProperty("p"+juce::String(i),getCurvePoint(i))); x.removeChild(c,nullptr); state.replaceState(x); } }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){ return new RONESidechainAudioProcessor(); }
