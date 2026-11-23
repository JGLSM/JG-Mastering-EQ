// Parameter safety: IDs never change by accident, everything recalls, and
// moving any control never clicks.
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "TestHelpers.h"

using namespace jgtest;

namespace
{
    juce::StringArray currentIds (JGMasteringEQAudioProcessor& p)
    {
        juce::StringArray ids;
        for (auto* ap : p.getParameters())
            if (auto* wp = dynamic_cast<juce::HostedAudioProcessorParameter*> (ap))
                ids.add (wp->getParameterID());
        return ids;
    }
}

TEST_CASE ("Parameter IDs match tests/parameter_ids.txt exactly", "[params]")
{
    // DAWs store automation and sessions against these IDs. If this test fails,
    // you changed, removed or reordered an ID. After v1 is locked, ONLY adding
    // new IDs at the end is allowed (and the file updated in the same change).
    JuceGuard guard;
    JGMasteringEQAudioProcessor proc;
    juce::StringArray expected;
    expected.addLines (juce::File (JGEQ_TESTS_DIR).getChildFile ("parameter_ids.txt").loadFileAsString());
    expected.removeEmptyStrings();
    for (int i = expected.size(); --i >= 0;)
        if (expected[i].startsWith ("#"))
            expected.remove (i);

    const auto ids = currentIds (proc);
    INFO ("Current IDs:\n" << ids.joinIntoString ("\n"));
    REQUIRE (ids == expected);
}

TEST_CASE ("Parameter IDs follow the brief's rules", "[params]")
{
    JuceGuard guard;
    JGMasteringEQAudioProcessor proc;
    juce::StringArray seen;
    for (auto* ap : proc.getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap);
        REQUIRE (rp != nullptr);
        const auto id = rp->getParameterID();
        CAPTURE (id);
        REQUIRE_FALSE (seen.contains (id));                       // unique
        seen.add (id);
        REQUIRE (id == id.toLowerCase());                          // rule 1: lowercase
        REQUIRE ((id.startsWith ("in_") || id.startsWith ("seq_") || id.startsWith ("peq_")
                  || id.startsWith ("out_") || id.startsWith ("glb_")));   // rule 1: stage prefix
        REQUIRE (rp->getVersionHint() >= 1);                       // rule 5: version hint
    }
}

TEST_CASE ("Session recall: random settings save and load exactly", "[params][state]")
{
    JuceGuard guard;
    const auto seed = GENERATE (1, 2, 3, 4, 5);
    JGMasteringEQAudioProcessor a;
    juce::Random rng (seed);
    for (auto* p : a.getParameters())
        p->setValueNotifyingHost (rng.nextFloat());

    juce::MemoryBlock saved;
    a.getStateInformation (saved);

    JGMasteringEQAudioProcessor b;
    b.setStateInformation (saved.getData(), (int) saved.getSize());

    for (int i = 0; i < a.getParameters().size(); ++i)
    {
        auto* pa = a.getParameters()[i];
        auto* pb = b.getParameters()[i];
        CAPTURE (pa->getName (64));
        // Compare what the plugin actually uses (switches and choices snap).
        auto* ra = dynamic_cast<juce::RangedAudioParameter*> (pa);
        const float legal = ra->convertTo0to1 (ra->convertFrom0to1 (pa->getValue()));
        REQUIRE (std::abs (pb->getValue() - legal) < 1.0e-6f);
    }

    // Save again from the reloaded copy and load into a third instance:
    // nothing may drift from one save to the next.
    juce::MemoryBlock resaved;
    b.getStateInformation (resaved);
    JGMasteringEQAudioProcessor c;
    c.setStateInformation (resaved.getData(), (int) resaved.getSize());
    for (int i = 0; i < b.getParameters().size(); ++i)
        REQUIRE (std::abs (c.getParameters()[i]->getValue() - b.getParameters()[i]->getValue()) < 1.0e-6f);
}

TEST_CASE ("Defaults: a fresh instance and a reset instance match", "[params][state]")
{
    JuceGuard guard;
    JGMasteringEQAudioProcessor fresh, used;
    for (auto* p : used.getParameters())
        p->setValueNotifyingHost (0.77f);
    for (auto* p : used.getParameters())
        p->setValueNotifyingHost (p->getDefaultValue());
    for (int i = 0; i < fresh.getParameters().size(); ++i)
        REQUIRE (std::abs (used.getParameters()[i]->getValue() - fresh.getParameters()[i]->getValue()) < 1.0e-6f);
}

TEST_CASE ("Display text round-trips (what the DAW shows can be typed back)", "[params]")
{
    JuceGuard guard;
    JGMasteringEQAudioProcessor proc;
    for (auto* ap : proc.getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap);
        for (float n : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
        {
            const auto text = rp->getText (n, 64);
            const auto back = rp->getValueForText (text);
            CAPTURE (rp->getParameterID(), n, text);
            // Within one display step: compare as displayed text.
            REQUIRE (rp->getText (back, 64) == text);
        }
    }
}

TEST_CASE ("Automation sweeps: moving any control fast causes no clicks", "[params][automation]")
{
    // Sweep each parameter end to end every block while a sine plays, then
    // compare the output's worst sample-to-sample jump with the input's.
    JuceGuard guard;
    const double sr = 48000.0;
    const auto in = sine (2, (int) sr, sr, 997.0, -6.0);
    const double inJump = worstSecondDifference (in);

    JGMasteringEQAudioProcessor proc;
    for (int pi = 0; pi < proc.getParameters().size(); ++pi)
    {
        auto* p = proc.getParameters()[pi];
        if (p->getName (64) == "Oversampling")
            continue;
        RenderOptions o { sr, 64 };
        int blockIndex = 0;
        o.beforeBlock = [&] (JGMasteringEQAudioProcessor&, int) { p->setValueNotifyingHost ((blockIndex++ % 2) ? 1.0f : 0.0f); };
        const auto out = render (proc, in, o);
        p->setValueNotifyingHost (p->getDefaultValue());

        CAPTURE (p->getName (64));
        // Allow a little headroom over the clean sine; a click is many times larger.
        REQUIRE (worstSecondDifference (out) <= inJump * 4.0 + 1e-6);
    }
}
