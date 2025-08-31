#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>

ViaUAudioProcessor::ViaUAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ViaUAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& in = layouts.getChannelSet(true, 0);
    const auto& out = layouts.getChannelSet(false, 0);
    if (in.isDisabled() || out.isDisabled()) return false;
    if (in.size() != out.size()) return false;
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}
#endif

void ViaUAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    fs = sampleRate;
    const double tau = 0.300; // 300 ms integration
    alpha = (float)std::exp(-1.0 / (tau * fs));
    vuIntegrator = 0.0f;
    currentVU.store(-20.0f);
    peakHit.store(false);
}

void ViaUAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);
    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();

    for (int n = 0; n < numSamples; ++n)
    {
        float sampleAbsMean = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            sampleAbsMean += std::abs(buffer.getReadPointer(ch)[n]);
        sampleAbsMean /= (float)juce::jmax(1, numCh);
        vuIntegrator = alpha * vuIntegrator + (1.0f - alpha) * sampleAbsMean;
    }

    constexpr float eps = 1.0e-9f;
    const float dbfs = juce::Decibels::gainToDecibels(vuIntegrator + eps);
    float vu = dbfs + 18.0f;
    vu = juce::jlimit(-20.0f, 3.0f, vu);
    currentVU.store(vu);

    peakHit.store(vu >= getPeakThreshold());
}

juce::AudioProcessorEditor* ViaUAudioProcessor::createEditor() { return new ViaUAudioProcessorEditor(*this); }

void ViaUAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void ViaUAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout ViaUAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "displayMode", "Display Mode", juce::StringArray{ "LED", "Needle" }, 0));
    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ViaUAudioProcessor(); }
