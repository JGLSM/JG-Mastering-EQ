#include "WebResources.h"
#include "JGUIData.h"

namespace jgeq::ui
{
namespace
{
    const char* mimeFor (const juce::String& file)
    {
        if (file.endsWith (".html")) return "text/html; charset=utf-8";
        if (file.endsWith (".js"))   return "text/javascript; charset=utf-8";
        if (file.endsWith (".css"))  return "text/css; charset=utf-8";
        if (file.endsWith (".woff2")) return "font/woff2";
        if (file.endsWith (".svg"))  return "image/svg+xml";
        if (file.endsWith (".png"))  return "image/png";
        return "application/octet-stream";
    }
}

std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url)
{
    auto path = url.upToFirstOccurrenceOf ("?", false, false).fromLastOccurrenceOf ("/", false, false);
    if (path.isEmpty())
        path = "index.html";

    for (int i = 0; i < JGUIData::namedResourceListSize; ++i)
    {
        const auto* name = JGUIData::namedResourceList[i];
        if (path == JGUIData::getNamedResourceOriginalFilename (name))
        {
            int size = 0;
            const auto* data = JGUIData::getNamedResource (name, size);
            std::vector<std::byte> bytes ((size_t) size);
            std::memcpy (bytes.data(), data, (size_t) size);
            return juce::WebBrowserComponent::Resource { std::move (bytes), mimeFor (path) };
        }
    }
    return std::nullopt;
}
}
