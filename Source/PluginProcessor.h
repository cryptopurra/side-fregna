#pragma once
#include <JuceHeader.h>
#include <array>

class RONESidechainAudioProcessor final : public juce::AudioProcessor {
public:
    RONESidechainAudioProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Side Fregna"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState state;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    void setCurvePoint(int index, float value) noexcept;
    float getCurvePoint(int index) const noexcept;
    void loadShape(int shape) noexcept;
private:
    float curveAt(float phase) const noexcept;
    float kickEnvelope = 0.0f, sampleRateHz = 44100.0f;
    int samplesSinceKick = 1000000;
    juce::AudioBuffer<float> delayed;
    int delayWrite = 0;
    std::array<std::atomic<float>, 33> curve{};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RONESidechainAudioProcessor)
};
