#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/PeakMeter.h"

// The audio engine. For now it is a pass-through: audio goes in and comes out
// untouched. The DSP stages (input, Smart EQ, passive EQ, output) will live in
// src/dsp/ so they can be tested without the UI or a DAW.
class JGMasteringEQAudioProcessor final : public juce::AudioProcessor
{
public:
    JGMasteringEQAudioProcessor();
    ~JGMasteringEQAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return true; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    // Parameter tree. IDs follow the rules in docs/BRIEF.md ("Parameters").
    // PROVISIONAL until the v1 list is reviewed and locked with Josh.
    juce::AudioProcessorValueTreeState apvts;

    // Peak levels for the IN and OUT meters (read by the editor).
    jgeq::dsp::PeakMeter inputMeter, outputMeter;

private:
    template <typename Sample>
    void processPassThrough (juce::AudioBuffer<Sample>&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JGMasteringEQAudioProcessor)
};
