#include "Parameters.h"

namespace jgeq::params
{
namespace
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    using APC = juce::AudioParameterChoice;

    juce::ParameterID pid (const juce::String& id) { return { id, kVersion }; }

    juce::NormalisableRange<float> logRange (float lo, float hi)
    {
        return { lo, hi,
                 [] (float start, float end, float n) { return start * std::pow (end / start, n); },
                 [] (float start, float end, float v) { return std::log (v / start) / std::log (end / start); },
                 [] (float, float, float v) { return v; } };
    }

    juce::String hzText (float hz, int)
    {
        // Brief: whole Hz below 1 kHz, 0.01 kHz steps above.
        return hz < 1000.0f ? juce::String (juce::roundToInt (hz)) + " Hz"
                            : juce::String (hz / 1000.0f, 2) + " kHz";
    }

    float hzFromText (const juce::String& t)
    {
        auto v = t.retainCharacters ("0123456789.").getFloatValue();
        return t.containsIgnoreCase ("k") ? v * 1000.0f : v;
    }

    juce::String dbText (float db, int)
    {
        return (db > 0.049f ? "+" : "") + juce::String (db, 1) + " dB";
    }

    auto floatAttr (std::function<juce::String (float, int)> toText, std::function<float (const juce::String&)> fromText = nullptr)
    {
        auto a = juce::AudioParameterFloatAttributes().withStringFromValueFunction (std::move (toText));
        if (fromText != nullptr)
            a = a.withValueFromStringFunction (std::move (fromText));
        return a;
    }
}

Taper taperFor (const juce::String& id)
{
    return (id.endsWith ("_freq") || id.endsWith ("_q")) ? Taper::log : Taper::linear;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ---- Stage 1: Input -----------------------------------------------------
    layout.add (std::make_unique<APF> (pid ("in_drive"), "Drive",
                                       juce::NormalisableRange<float> (0.0f, 10.0f, 0.1f), 3.0f,
                                       floatAttr ([] (float v, int) { return juce::String (v, 1); })));
    layout.add (std::make_unique<APB> (pid ("in_bypass"), "Input Bypass", false));
    layout.add (std::make_unique<APB> (pid ("in_lift"), "Lift", false));

    // ---- Stage 2: Smart EQ --------------------------------------------------
    layout.add (std::make_unique<APF> (pid ("seq_amount"), "Smart Amount",
                                       juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 30.0f,
                                       floatAttr ([] (float v, int) { return juce::String (juce::roundToInt (v)) + "%"; })));
    layout.add (std::make_unique<APC> (pid ("seq_range"), "Smart Range", kRangeChoices, 1));
    layout.add (std::make_unique<APB> (pid ("seq_cut_only"), "Cut Only", true));
    layout.add (std::make_unique<APB> (pid ("seq_freeze"), "Freeze", false));
    layout.add (std::make_unique<APB> (pid ("seq_delta"), "Delta Listen", false));
    layout.add (std::make_unique<APB> (pid ("seq_bypass"), "Smart EQ Bypass", false));

    // ---- Stage 3: Passive EQ ------------------------------------------------
    for (int b = 0; b < kNumBands; ++b)
    {
        const auto& d = kBands[b];
        const juce::String n (b + 1);
        layout.add (std::make_unique<APF> (pid (bandId (b, "freq")), "Band " + n + " Freq",
                                           logRange (d.minHz, d.maxHz), d.defaultHz, floatAttr (hzText, hzFromText)));
        layout.add (std::make_unique<APF> (pid (bandId (b, "gain")), "Band " + n + " Gain",
                                           juce::NormalisableRange<float> (-10.0f, 10.0f, 0.1f), 0.0f, floatAttr (dbText)));
        auto qRange = logRange (0.3f, 4.0f);
        qRange.interval = 0.0f;
        layout.add (std::make_unique<APF> (pid (bandId (b, "q")), "Band " + n + " Q",
                                           qRange, d.defaultQ,
                                           floatAttr ([] (float v, int) { return juce::String (v, 1); })));
        layout.add (std::make_unique<APC> (pid (bandId (b, "shape")), "Band " + n + " Shape", kShapeChoices, d.defaultShape));
    }
    layout.add (std::make_unique<APC> (pid ("peq_sat"), "Saturation Mode", kSatChoices, 0));
    layout.add (std::make_unique<APB> (pid ("peq_bypass"), "Passive EQ Bypass", false));

    // ---- Output / global ----------------------------------------------------
    layout.add (std::make_unique<APF> (pid ("out_gain"), "Output Gain",
                                       juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, floatAttr (dbText)));
    layout.add (std::make_unique<APC> (pid ("out_target"), "Loudness Target", kTargetChoices, 2));
    layout.add (std::make_unique<APB> (pid ("glb_bypass"), "Bypass", false));
    layout.add (std::make_unique<APC> (pid ("glb_oversampling"), "Oversampling", kOsChoices, 1));

    return layout;
}
}
