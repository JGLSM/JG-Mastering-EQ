#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

namespace jgeq::dsp
{
// Collects the highest sample level per channel on the audio thread; the
// interface reads (and resets) it about 30 times a second. Lock-free.
class PeakMeter
{
public:
    static constexpr int kChannels = 2;

    template <typename Sample>
    void process (const juce::AudioBuffer<Sample>& buffer) noexcept
    {
        const int n = buffer.getNumSamples();
        const int chans = buffer.getNumChannels();
        for (int ch = 0; ch < kChannels; ++ch)
        {
            const int src = juce::jmin (ch, chans - 1);   // mono feeds both bars
            if (src < 0)
                continue;
            const auto mag = (float) buffer.getMagnitude (src, 0, n);
            auto& p = peaks[(size_t) ch];
            float prev = p.load (std::memory_order_relaxed);
            while (mag > prev && ! p.compare_exchange_weak (prev, mag, std::memory_order_relaxed)) {}
        }
    }

    // Returns the peak (linear gain) since the last call, then resets it.
    float takePeak (int ch) noexcept { return peaks[(size_t) ch].exchange (0.0f, std::memory_order_relaxed); }

private:
    std::array<std::atomic<float>, kChannels> peaks {};
};
}
