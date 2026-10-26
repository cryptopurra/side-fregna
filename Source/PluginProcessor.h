#pragma once
#include <JuceHeader.h>
#include <array>
#include <cstdint>
#include <vector>

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
    double getTailLengthSeconds() const override { return 0.01; }
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
    bool loadKickSample(const juce::File& file);
    juce::String getKickSampleName() const;
private:
    float curveAt(float phase) const noexcept;
    float kickEnvelope = 0.0f, sampleRateHz = 44100.0f, spaceEnvelope = 0.0f;
    int kickAge = 0, kickCountdown = -1, delaySamples = 0, delayWrite = 0;
    juce::AudioBuffer<float> delayed;
    std::array<float, 2> lowState{}, highState{}, phaseX1{}, phaseY1{}, eqX1{}, eqX2{}, eqY1{}, eqY2{};
    float phaseA = 0.0f, eqB0 = 1.0f, eqB1 = 0.0f, eqB2 = 0.0f, eqA1 = 0.0f, eqA2 = 0.0f;
    std::array<std::atomic<float>, 33> curve{};
    std::atomic<juce::AudioBuffer<float>*> kickSample{nullptr};
    std::vector<std::unique_ptr<juce::AudioBuffer<float>>> kickSampleVersions;
    juce::String kickSampleName;
    int kickSamplePosition = -1;
    std::atomic<bool> kickSampleChanged{false};
    std::int64_t externalQuietSamples = 0;
    bool hasExternalKick = false;
    int lastGridPhase = -1;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RONESidechainAudioProcessor)
};
