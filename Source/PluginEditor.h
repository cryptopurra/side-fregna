#pragma once
#include "PluginProcessor.h"
class RONESidechainAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit RONESidechainAudioProcessorEditor(RONESidechainAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
private:
    void timerCallback() override { repaint(); }
    RONESidechainAudioProcessor& processor;
    juce::ComboBox page, division, mode;
    juce::ComboBox shape;
    juce::Slider depth, click, split, phase, spaceAmount, release, spaceFrequency;
    juce::ToggleButton space, phaseEnabled;
    juce::Label title, hint;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionLink, modeLink;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeLink;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthLink, clickLink, phaseLink, splitLink, amountLink, releaseLink;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> spaceLink, phaseEnabledLink;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> frequencyLink;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RONESidechainAudioProcessorEditor)
};
