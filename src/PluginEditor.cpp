#include "PluginEditor.h"
#include "Parameters.h"
#include "ui/WebResources.h"

namespace
{
    constexpr int kDesignW = 1360, kDesignH = 662;

    juce::var describe (juce::RangedAudioParameter& p)
    {
        auto* o = new juce::DynamicObject();
        const auto& r = p.getNormalisableRange();
        o->setProperty ("id", p.getParameterID());
        o->setProperty ("def", p.convertFrom0to1 (p.getDefaultValue()));
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (&p))
        {
            o->setProperty ("kind", "choice");
            o->setProperty ("min", 0);
            o->setProperty ("max", c->choices.size() - 1);
            juce::Array<juce::var> names;
            for (auto& s : c->choices)
                names.add (s);
            o->setProperty ("choices", names);
        }
        else if (dynamic_cast<juce::AudioParameterBool*> (&p) != nullptr)
        {
            o->setProperty ("kind", "bool");
            o->setProperty ("min", 0);
            o->setProperty ("max", 1);
        }
        else
        {
            o->setProperty ("kind", "float");
            o->setProperty ("min", r.start);
            o->setProperty ("max", r.end);
            o->setProperty ("interval", r.interval);
            o->setProperty ("map", jgeq::params::taperFor (p.getParameterID()) == jgeq::params::Taper::log ? "log" : "lin");
        }
        return o;
    }

    float plainValue (juce::RangedAudioParameter& p) { return p.convertFrom0to1 (p.getValue()); }

    float toDb (float gain) { return gain > 0.0f ? juce::jmax (-120.0f, 20.0f * std::log10 (gain)) : -120.0f; }
}

JGMasteringEQAudioProcessorEditor::JGMasteringEQAudioProcessorEditor (JGMasteringEQAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    for (auto* ap : p.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (ap))
            params.push_back (rp);
    lastSent.assign (params.size(), std::numeric_limits<float>::quiet_NaN());

    auto options = juce::WebBrowserComponent::Options {}
                       .withNativeIntegrationEnabled()
                       .withKeepPageLoadedWhenBrowserIsHidden()
                       .withResourceProvider ([] (const auto& url) { return jgeq::ui::getResource (url); })
                       .withEventListener ("jg_ready", [this] (const juce::var&) { sendInit(); })
                       .withEventListener ("jg_set", [this] (const juce::var& v) { handleSet (v); })
                       .withEventListener ("jg_gesture", [this] (const juce::var& v) { handleGesture (v); });
#if JUCE_WINDOWS
    options = options.withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
                     .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2 {}
                                                  .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)));
#endif

    web = std::make_unique<juce::WebBrowserComponent> (options);
    addAndMakeVisible (*web);
    web->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Start at 80% of the design size; resizable with the design's aspect ratio.
    setResizable (true, true);
    setResizeLimits (kDesignW / 2, kDesignH / 2, kDesignW * 2, kDesignH * 2);
    getConstrainer()->setFixedAspectRatio ((double) kDesignW / kDesignH);
    setSize (kDesignW * 4 / 5, kDesignH * 4 / 5);

    startTimerHz (30);
}

JGMasteringEQAudioProcessorEditor::~JGMasteringEQAudioProcessorEditor()
{
    stopTimer();
}

void JGMasteringEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0a0a0b));
}

void JGMasteringEQAudioProcessorEditor::resized()
{
    web->setBounds (getLocalBounds());
}

juce::RangedAudioParameter* JGMasteringEQAudioProcessorEditor::findParam (const juce::var& payload) const
{
    const auto id = payload.getProperty ("id", {}).toString();
    for (auto* p : params)
        if (p->getParameterID() == id)
            return p;
    return nullptr;
}

void JGMasteringEQAudioProcessorEditor::handleSet (const juce::var& payload)
{
    if (auto* p = findParam (payload))
    {
        const auto plain = (float) payload.getProperty ("value", 0.0);
        p->setValueNotifyingHost (p->convertTo0to1 (plain));
        // Remember what the page already shows so we don't echo it back.
        for (size_t i = 0; i < params.size(); ++i)
            if (params[i] == p)
                lastSent[i] = plainValue (*p);
    }
}

void JGMasteringEQAudioProcessorEditor::handleGesture (const juce::var& payload)
{
    if (auto* p = findParam (payload))
    {
        if ((bool) payload.getProperty ("begin", false))
            p->beginChangeGesture();
        else
            p->endChangeGesture();
    }
}

void JGMasteringEQAudioProcessorEditor::sendInit()
{
    juce::Array<juce::var> meta;
    auto* values = new juce::DynamicObject();
    for (size_t i = 0; i < params.size(); ++i)
    {
        meta.add (describe (*params[i]));
        lastSent[i] = plainValue (*params[i]);
        values->setProperty (params[i]->getParameterID(), lastSent[i]);
    }
    auto* o = new juce::DynamicObject();
    o->setProperty ("meta", meta);
    o->setProperty ("values", juce::var (values));
    web->emitEventIfBrowserIsVisible ("jg_init", juce::var (o));
    pageReady = true;
}

void JGMasteringEQAudioProcessorEditor::timerCallback()
{
    if (! pageReady)
        return;

    // Parameter changes from the host (automation, presets, undo) -> page.
    juce::DynamicObject::Ptr changed = new juce::DynamicObject();
    for (size_t i = 0; i < params.size(); ++i)
    {
        const auto v = plainValue (*params[i]);
        if (! juce::exactlyEqual (v, lastSent[i]))
        {
            lastSent[i] = v;
            changed->setProperty (params[i]->getParameterID(), v);
        }
    }
    if (! changed->getProperties().isEmpty())
        web->emitEventIfBrowserIsVisible ("jg_params", juce::var (changed.get()));

    // Meters: peak since last tick, with a gentle fall-off.
    constexpr float fallDbPerTick = 1.5f;
    float now[4] = { toDb (processor.inputMeter.takePeak (0)), toDb (processor.inputMeter.takePeak (1)),
                     toDb (processor.outputMeter.takePeak (0)), toDb (processor.outputMeter.takePeak (1)) };
    for (int i = 0; i < 4; ++i)
        meterDb[i] = juce::jmax (now[i], meterDb[i] - fallDbPerTick);

    auto* m = new juce::DynamicObject();
    m->setProperty ("in", juce::Array<juce::var> { meterDb[0], meterDb[1] });
    m->setProperty ("out", juce::Array<juce::var> { meterDb[2], meterDb[3] });
    web->emitEventIfBrowserIsVisible ("jg_meters", juce::var (m));
}
