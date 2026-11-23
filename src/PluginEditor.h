#pragma once

#include "PluginProcessor.h"

// The plugin window. It hosts a web view that shows the C13 "Ice Grey"
// interface (files in ui/, compiled into the plugin) and passes parameter
// changes and meter levels back and forth.
class JGMasteringEQAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                private juce::Timer
{
public:
    explicit JGMasteringEQAudioProcessorEditor (JGMasteringEQAudioProcessor&);
    ~JGMasteringEQAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void handleSet (const juce::var& payload);
    void handleGesture (const juce::var& payload);
    void sendInit();
    juce::RangedAudioParameter* findParam (const juce::var& payload) const;

    JGMasteringEQAudioProcessor& processor;
    std::vector<juce::RangedAudioParameter*> params;
    std::vector<float> lastSent;     // last value sent to the page, per parameter
    bool pageReady = false;
    float meterDb[4] { -120.0f, -120.0f, -120.0f, -120.0f };

    std::unique_ptr<juce::WebBrowserComponent> web;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JGMasteringEQAudioProcessorEditor)
};
