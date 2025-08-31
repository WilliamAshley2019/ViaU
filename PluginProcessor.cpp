#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_dsp/juce_dsp.h>

ViaUAudioProcessor::ViaUAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

void ViaUAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    fs = sampleRate;

    // 300 ms VU integration time constant (single-pole low-pass on rectified signal)
    const double tau = 0.300; // seconds
    alpha = (float)std::exp(-1.0 / (tau * fs));

    vuIntegrator = 0.0f;
    currentVU.store(-20.0f);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ViaUAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Allow mono or stereo input/output, and require matching sizes.
    const auto& in = layouts.getChannelSet(true, 0);
    const auto& out = layouts.getChannelSet(false, 0);

    if (in.isDisabled() || out.isDisabled())
        return false;

    if (in.size() != out.size())
        return false;

    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}
#endif

void ViaUAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);

    // Pass-through audio unchanged
    // (No processing besides level detection)
    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();

    // Compute rectified absolute signal averaged across channels
    // and integrate with ~300 ms time constant for classic VU ballistics.
    for (int n = 0; n < numSamples; ++n)
    {
        float sampleAbsMean = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            sampleAbsMean += std::abs(buffer.getReadPointer(ch)[n]);

        sampleAbsMean /= (float)juce::jmax(1, numCh);

        // Single-pole IIR: y[n] = alpha*y[n-1] + (1-alpha)*x[n]
        vuIntegrator = alpha * vuIntegrator + (1.0f - alpha) * sampleAbsMean;
    }

    // Convert to dBFS for reference mapping.
    // Add a tiny epsilon to avoid log of zero.
    constexpr float eps = 1.0e-9f;
    const float dbfs = juce::Decibels::gainToDecibels(vuIntegrator + eps); // negative value in dBFS

    // Map 0 VU ≈ -18 dBFS (common digital reference).
    float vu = dbfs + 18.0f;

    // Clamp to a sensible VU meter range (-20 .. +3 VU)
    vu = juce::jlimit(-20.0f, 3.0f, vu);

    currentVU.store(vu);

    // Ensure we leave audio untouched.
    // (No need to explicitly copy; we didn't modify buffer data.)
}

juce::AudioProcessorEditor* ViaUAudioProcessor::createEditor()
{
    return new ViaUAudioProcessorEditor(*this);
}

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

    // Display mode: 0 = LED, 1 = Needle
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "displayMode", "Display Mode", juce::StringArray{ "LED", "Needle" }, 0));

    return { params.begin(), params.end() };
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ViaUAudioProcessor();
}