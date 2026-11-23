#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// PROVISIONAL parameter list (draft v0 from docs/BRIEF.md, "Parameters").
//
// These IDs are NOT locked yet. They exist so the prototype interface can be
// clicked, automated and saved. Sessions saved with prototype builds may not
// recall in later builds. Once Josh signs off the list, IDs become permanent:
// never rename, delete or reuse one after that (see the ID rules in the brief).
namespace jgeq::params
{
    // Every parameter uses this version hint (ID rule 5). Parameters added in
    // later releases must use a higher number.
    inline constexpr int kVersion = 1;

    struct BandDef
    {
        const char* name;
        float defaultHz, minHz, maxHz;
        int defaultShape;   // index into kShapeChoices
        float defaultQ;
    };

    // Provisional frequencies and ranges (brief: "all frequencies and sweep
    // ranges are provisional").
    inline constexpr BandDef kBands[] = {
        { "Sub",      40.0f,    16.0f,   100.0f,   1, 0.7f },
        { "Low",      100.0f,   40.0f,   300.0f,   0, 0.8f },
        { "Low Mid",  300.0f,   150.0f,  1000.0f,  0, 1.2f },
        { "Mid",      1000.0f,  500.0f,  4000.0f,  0, 1.0f },
        { "High Mid", 3000.0f,  1500.0f, 6000.0f,  0, 1.4f },
        { "Presence", 6000.0f,  3000.0f, 12000.0f, 0, 2.0f },
        { "Air",      16000.0f, 6000.0f, 24000.0f, 2, 0.7f },
    };
    inline constexpr int kNumBands = (int) std::size (kBands);

    inline const juce::StringArray kShapeChoices  { "Bell", "Low Shelf", "High Shelf" };
    inline const juce::StringArray kSatChoices    { "Tube", "Iron", "SS", "Tape" };
    inline const juce::StringArray kRangeChoices  { juce::String::fromUTF8 ("\xc2\xb1" "1.5 dB"), juce::String::fromUTF8 ("\xc2\xb1" "3 dB") };
    // "\xe2\x88\x92" is the typographic minus sign (UTF-8).
    inline const juce::StringArray kTargetChoices { juce::String::fromUTF8 ("\xe2\x88\x92" "23 LUFS"), juce::String::fromUTF8 ("\xe2\x88\x92" "16 LUFS"),
                                                    juce::String::fromUTF8 ("\xe2\x88\x92" "14 LUFS"), juce::String::fromUTF8 ("\xe2\x88\x92" "11 LUFS"),
                                                    juce::String::fromUTF8 ("\xe2\x88\x92" "9 LUFS") };
    inline const juce::StringArray kOsChoices     { "1x", "2x", "4x" };

    inline juce::String bandId (int band, const char* suffix)
    {
        return "peq_b" + juce::String (band + 1) + "_" + suffix;
    }

    // How a parameter maps to a knob, so the interface can match the DSP side.
    enum class Taper { linear, log };

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Taper used by a float parameter (log for frequency and Q).
    Taper taperFor (const juce::String& id);
}
