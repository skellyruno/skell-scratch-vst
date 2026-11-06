#pragma once
#include <JuceHeader.h>
namespace Theme {
inline const auto bg       = juce::Colour::fromRGB(9, 14, 12);
inline const auto panel    = juce::Colour::fromRGB(17, 25, 21);
inline const auto elevated = juce::Colour::fromRGB(28, 41, 33);
inline const auto plot     = juce::Colour::fromRGB(11, 20, 16);
inline const auto accent   = juce::Colour::fromRGB(117, 250, 104);
inline const auto accent2  = juce::Colour::fromRGB(48, 182, 105);
inline const auto edge     = juce::Colour::fromRGB(53, 82, 64);
inline const auto muted    = juce::Colour::fromRGB(150, 174, 158);
inline const auto light    = juce::Colour::fromRGB(223, 237, 227);
inline const auto white    = juce::Colour::fromRGB(244, 255, 244);
inline juce::Font font(float height, bool bold = false) {
 // Consistent modern italic display typography without an external font file.
 return juce::Font(juce::FontOptions().withHeight(height).withStyle(bold ? "Bold Italic" : "Italic"));
}
// A readable native sans for small metadata; display text uses a clean italic variant.
inline juce::Font uiFont(float height, bool bold = false) {
 return juce::Font(juce::FontOptions().withHeight(height).withStyle(bold ? "Bold" : "Regular"));
}
}
