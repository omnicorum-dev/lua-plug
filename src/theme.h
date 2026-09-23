#pragma once

#include "juce_graphics/juce_graphics.h"
#include <BinaryData.h>

static constexpr float text_height = 30.f;

struct Theme {
    juce::Colour background{0xff181818};
    juce::Colour panel{0xff323232};
    juce::Colour deselected{0xff686868};
    juce::Colour hovered{0xff999999};
    juce::Colour accent{0xff3593ff};
    juce::Colour text{0xffe0e0e0};
    juce::Colour text_dim{0xff707070};

    float round_radius = 10.f;
};

namespace Fonts {

inline juce::Typeface::Ptr jetBrainsMono() {
    static auto typeface = juce::Typeface::createSystemTypefaceFor(
        BinaryData::JetBrainsMonoRegular_ttf,
        BinaryData::JetBrainsMonoRegular_ttfSize);

    return typeface;
}

inline juce::Typeface::Ptr jetBrainsMonoBold() {
    static auto typeface = juce::Typeface::createSystemTypefaceFor(
        BinaryData::JetBrainsMonoBold_ttf,
        BinaryData::JetBrainsMonoBold_ttfSize);
    return typeface;
}

inline juce::Font jetBrainsMono(float height) {
    return juce::Font(juce::FontOptions{jetBrainsMono()}.withHeight(height));
}

inline juce::Font jetBrainsMonoBold(float height) {
    return juce::Font(
        juce::FontOptions{jetBrainsMonoBold()}.withHeight(height));
}

} // namespace Fonts
