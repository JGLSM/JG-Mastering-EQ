#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "PluginProcessor.h"

namespace
{
    // Deterministic noise so failures are reproducible.
    juce::AudioBuffer<float> makeNoise (int channels, int samples, juce::int64 seed = 1234)
    {
        juce::Random rng (seed);
        juce::AudioBuffer<float> b (channels, samples);
        for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < samples; ++i)
                b.setSample (ch, i, rng.nextFloat() * 2.0f - 1.0f);
        return b;
    }

    struct JuceGuard { juce::ScopedJuceInitialiser_GUI init; };
}

TEST_CASE ("Null test: default state leaves audio bit-identical", "[null]")
{
    JuceGuard guard;
    const auto sampleRate = GENERATE (44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0);
    const auto blockSize  = GENERATE (1, 64, 441, 512, 2048);

    JGMasteringEQAudioProcessor proc;
    proc.setPlayConfigDetails (2, 2, sampleRate, blockSize);
    proc.prepareToPlay (sampleRate, blockSize);

    const int totalSamples = 8192;
    auto reference = makeNoise (2, totalSamples);
    auto processed = reference;

    juce::MidiBuffer midi;
    for (int start = 0; start < totalSamples; start += blockSize)
    {
        const int n = std::min (blockSize, totalSamples - start);
        juce::AudioBuffer<float> view (processed.getArrayOfWritePointers(), 2, start, n);
        proc.processBlock (view, midi);
    }

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < totalSamples; ++i)
            REQUIRE (processed.getSample (ch, i) == reference.getSample (ch, i));

    REQUIRE (proc.getLatencySamples() == 0);
    proc.releaseResources();
}

TEST_CASE ("State save and load round-trips", "[state]")
{
    JuceGuard guard;
    JGMasteringEQAudioProcessor a;
    juce::MemoryBlock saved;
    a.getStateInformation (saved);
    REQUIRE (saved.getSize() > 0);

    JGMasteringEQAudioProcessor b;
    b.setStateInformation (saved.getData(), (int) saved.getSize());

    juce::MemoryBlock resaved;
    b.getStateInformation (resaved);
    REQUIRE (saved == resaved);
}

TEST_CASE ("Bus layouts: mono and stereo only, matching in and out", "[layout]")
{
    JuceGuard guard;
    JGMasteringEQAudioProcessor proc;
    using CS = juce::AudioChannelSet;
    auto layout = [] (CS in, CS out)
    {
        juce::AudioProcessor::BusesLayout l;
        l.inputBuses.add (in);
        l.outputBuses.add (out);
        return l;
    };
    REQUIRE (proc.checkBusesLayoutSupported (layout (CS::stereo(), CS::stereo())));
    REQUIRE (proc.checkBusesLayoutSupported (layout (CS::mono(), CS::mono())));
    REQUIRE_FALSE (proc.checkBusesLayoutSupported (layout (CS::mono(), CS::stereo())));
    REQUIRE_FALSE (proc.checkBusesLayoutSupported (layout (CS::create5point1(), CS::create5point1())));
}
