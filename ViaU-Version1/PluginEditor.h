#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class ViaUAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    explicit ViaUAudioProcessorEditor(ViaUAudioProcessor&);
    ~ViaUAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    void drawLedMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float vu);
    void drawNeedleMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float vu);

    ViaUAudioProcessor& processor;

    // Cached values for painting
    float vuValue = -20.0f; // in VU units

    // Parameter attachment for display mode
    juce::ComboBox displayModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> displayModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViaUAudioProcessorEditor)
};