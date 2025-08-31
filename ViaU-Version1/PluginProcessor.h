#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

class ViaUAudioProcessor : public juce::AudioProcessor
{
public:
    ViaUAudioProcessor();
    ~ViaUAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Expose current VU level (in VU units) to the editor thread.
    float getCurrentVU() const noexcept { return currentVU.load(); }

    // Parameter layout / state
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    // VU integration
    double fs = 44100.0;
    float vuIntegrator = 0.0f;          // internal smoothed value (linear)
    float alpha = 0.0f;                 // per-sample smoothing coeff from 300 ms time constant
    std::atomic<float> currentVU{ -20.0f }; // exposed VU value in VU units (-20..+3)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViaUAudioProcessor)
};