#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

JGMasteringEQAudioProcessor::JGMasteringEQAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "JGMasteringEQState", jgeq::params::createLayout())
{
}

juce::AudioProcessorParameter* JGMasteringEQAudioProcessor::getBypassParameter() const
{
    return apvts.getParameter ("glb_bypass");
}

void JGMasteringEQAudioProcessor::prepareToPlay (double, int)
{
    setLatencySamples (0);
}

bool JGMasteringEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

template <typename Sample>
void JGMasteringEQAudioProcessor::processPassThrough (juce::AudioBuffer<Sample>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    // Clear any output channels that have no matching input.
    for (auto ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    inputMeter.process (buffer);
    // Pass-through: the audio is left exactly as it arrived. DSP stages go here.
    outputMeter.process (buffer);
}

void JGMasteringEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    processPassThrough (buffer);
}

void JGMasteringEQAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    processPassThrough (buffer);
}

juce::AudioProcessorEditor* JGMasteringEQAudioProcessor::createEditor()
{
    return new JGMasteringEQAudioProcessorEditor (*this);
}

void JGMasteringEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void JGMasteringEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JGMasteringEQAudioProcessor();
}
