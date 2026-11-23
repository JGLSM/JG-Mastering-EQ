#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace jgeq::ui
{
// Serves the interface files (ui/ folder, compiled into the plugin as binary
// data) to the web view.
std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);
}
