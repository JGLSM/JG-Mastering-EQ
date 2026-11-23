// CPU benchmark: how much of one CPU core a single instance uses at common
// sample rates, at default settings and with every band boosted. Not a
// pass/fail test; CI prints it so changes can be tracked over time.
//
// Usage: JGMasteringEQBench [seconds-of-audio]   (default 20)
#include "PluginProcessor.h"
#include <chrono>
#include <cstdio>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const double seconds = argc > 1 ? std::atof (argv[1]) : 20.0;
    const int block = 512;

    std::printf ("JG Mastering EQ CPU benchmark (%.0f s of stereo audio per case, block %d)\n", seconds, block);
    std::printf ("%-10s %-14s %10s %10s\n", "rate", "settings", "x realtime", "CPU %");

    for (double sr : { 44100.0, 96000.0, 192000.0 })
    {
        for (int heavy = 0; heavy <= 1; ++heavy)
        {
            JGMasteringEQAudioProcessor p;
            if (heavy)
                for (int b = 1; b <= 7; ++b)
                    if (auto* g = p.apvts.getParameter ("peq_b" + juce::String (b) + "_gain"))
                        g->setValueNotifyingHost (1.0f);
            p.setPlayConfigDetails (2, 2, sr, block);
            p.prepareToPlay (sr, block);

            juce::AudioBuffer<float> buf (2, block);
            juce::Random rng (1);
            juce::MidiBuffer midi;
            const int blocks = (int) (seconds * sr / block);
            const auto t0 = std::chrono::steady_clock::now();
            for (int i = 0; i < blocks; ++i)
            {
                for (int ch = 0; ch < 2; ++ch)
                    for (int s = 0; s < block; ++s)
                        buf.setSample (ch, s, rng.nextFloat() * 0.5f - 0.25f);
                p.processBlock (buf, midi);
            }
            const double elapsed = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
            const double audio = blocks * (double) block / sr;
            std::printf ("%-10.0f %-14s %10.1f %9.3f%%\n", sr, heavy ? "all bands +10" : "default",
                         audio / elapsed, 100.0 * elapsed / audio);
            p.releaseResources();
        }
    }
    return 0;
}
