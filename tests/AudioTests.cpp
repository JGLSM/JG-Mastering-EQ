// Sound-level checks. With the prototype's pass-through these confirm the
// plugin is perfectly clean; as DSP lands, each stage adds its own cases here
// (band curves vs goldens, saturation THD and aliasing, loudness meters vs
// EBU reference files).
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "TestHelpers.h"

using namespace jgtest;

TEST_CASE ("Frequency response at null settings is flat (golden, +/-0.05 dB)", "[response]")
{
    JuceGuard guard;
    JGMasteringEQAudioProcessor proc;
    applySettings (proc, nullSettings());
    const auto curve = frequencyResponseDb (proc, logFrequencies (64), 48000.0, 16, proc.getLatencySamples());
    for (double v : curve)
        REQUIRE (std::abs (v) <= 0.05);
    REQUIRE (compareCurveGolden ("response_null_48k", curve) <= 0.05);
}

TEST_CASE ("Clean at null settings: no harmonics, no aliasing", "[distortion]")
{
    JuceGuard guard;
    const double sr = 48000.0;
    const auto hz = coherentHz (GENERATE (100.0, 1000.0, 9000.0), sr);
    JGMasteringEQAudioProcessor proc;
    applySettings (proc, nullSettings());
    const auto out = render (proc, sine (1, 1 << 16, sr, hz, -1.0), { sr, 512 });
    CAPTURE (hz);
    REQUIRE (thdDb (out, sr, hz) < -120.0);
    REQUIRE (nonHarmonicDb (out, sr, hz) < -120.0);
}

TEST_CASE ("Drive is level-matched (never wins an A/B by being louder)", "[level]")
{
    // Brief: Drive has automatic level compensation. Loudness at any Drive
    // setting must stay within 0.5 dB of Drive at zero.
    JuceGuard guard;
    const double sr = 48000.0;
    const auto in = noise (2, (int) sr * 2, 0.25);
    JGMasteringEQAudioProcessor proc;
    setPlain (proc, "in_drive", 0.0f);
    const double ref = rmsDb (render (proc, in, { sr, 512 }));
    for (float d : { 1.0f, 3.0f, 5.0f, 7.5f, 10.0f })
    {
        setPlain (proc, "in_drive", d);
        const double lvl = rmsDb (render (proc, in, { sr, 512 }));
        CAPTURE (d, lvl - ref);
        REQUIRE (std::abs (lvl - ref) <= 0.5);
    }
}

TEST_CASE ("Golden render: default settings match the stored render", "[golden]")
{
    // Fixed test material through the default settings, compared with
    // tests/golden/render_default.wav. When a sound change is approved by ear,
    // refresh with JGEQ_UPDATE_GOLDENS=1.
    JuceGuard guard;
    const double sr = 48000.0;
    auto material = logSweep (2, (int) (sr * 0.5), sr, 20.0, 20000.0, -6.0);
    const auto n = noise (2, material.getNumSamples(), 0.1, 99);
    for (int ch = 0; ch < 2; ++ch)
        material.addFrom (ch, 0, n, ch, 0, material.getNumSamples());

    JGMasteringEQAudioProcessor proc;
    const auto out = render (proc, material, { sr, 512 });
    REQUIRE (compareRenderGolden ("render_default", out, sr) <= -120.0);
}
