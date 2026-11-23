// Basic plugin behaviour: null test, bus layouts, latency.
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "TestHelpers.h"

using namespace jgtest;

TEST_CASE ("Null test: null settings leave audio bit-identical", "[null]")
{
    JuceGuard guard;
    const auto sampleRate = GENERATE (44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0);
    const auto blockSize  = GENERATE (1, 64, 441, 512, 2048);

    JGMasteringEQAudioProcessor proc;
    applySettings (proc, nullSettings());
    const auto in = noise (2, 8192);
    const auto out = render (proc, in, { sampleRate, blockSize });

    CAPTURE (sampleRate, blockSize);
    REQUIRE (nullDepthDb (in, out, proc.getLatencySamples()) <= -400.0);
}

TEST_CASE ("Null test: double precision and offline bounce", "[null]")
{
    JuceGuard guard;
    JGMasteringEQAudioProcessor proc;
    applySettings (proc, nullSettings());

    const auto in64 = noise<double> (2, 8192);
    REQUIRE (nullDepthDb (in64, render (proc, in64, { 96000.0, 256 }), proc.getLatencySamples()) <= -400.0);

    const auto in = noise (2, 8192);
    RenderOptions offline { 48000.0, 1024, true };
    REQUIRE (nullDepthDb (in, render (proc, in, offline), proc.getLatencySamples()) <= -400.0);
}

TEST_CASE ("Null test: mono", "[null]")
{
    JuceGuard guard;
    JGMasteringEQAudioProcessor proc;
    juce::AudioProcessor::BusesLayout mono;
    mono.inputBuses.add (juce::AudioChannelSet::mono());
    mono.outputBuses.add (juce::AudioChannelSet::mono());
    REQUIRE (proc.setBusesLayout (mono));
    applySettings (proc, nullSettings());
    const auto in = noise (1, 8192);
    REQUIRE (nullDepthDb (in, render (proc, in, { 48000.0, 512 }), proc.getLatencySamples()) <= -400.0);
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

TEST_CASE ("Latency is fixed: it never changes with settings during a session", "[latency]")
{
    // Brief (Decided 23 Nov 2026): fixed realtime oversampling, so the latency
    // reported to the DAW must not move when controls move.
    JuceGuard guard;
    const auto sampleRate = GENERATE (44100.0, 96000.0, 192000.0);
    JGMasteringEQAudioProcessor proc;
    proc.setPlayConfigDetails (2, 2, sampleRate, 512);
    proc.prepareToPlay (sampleRate, 512);
    const int reported = proc.getLatencySamples();

    juce::Random rng (7);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    for (int i = 0; i < 200; ++i)
    {
        auto* p = proc.getParameters()[rng.nextInt (proc.getParameters().size())];
        if (p->getName (64) == "Oversampling")
            continue;   // offline/realtime factor switching is tested separately once it exists
        p->setValueNotifyingHost (rng.nextFloat());
        buf.clear();
        proc.processBlock (buf, midi);
        REQUIRE (proc.getLatencySamples() == reported);
    }
}

TEST_CASE ("Silence in, silence out (no noise floor, no DC)", "[null]")
{
    JuceGuard guard;
    JGMasteringEQAudioProcessor proc;
    juce::AudioBuffer<float> in (2, 48000);
    in.clear();
    const auto out = render (proc, in, { 48000.0, 512 });
    REQUIRE (peakDb (out) <= -200.0);
}
