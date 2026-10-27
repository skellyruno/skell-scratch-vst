#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>
namespace Theme {
inline const auto bg       = juce::Colour::fromRGB(8, 12, 11);
inline const auto panel    = juce::Colour::fromRGB(17, 23, 20);
inline const auto elevated = juce::Colour::fromRGB(24, 32, 27);
inline const auto plot     = juce::Colour::fromRGB(12, 18, 15);
inline const auto accent   = juce::Colour::fromRGB(112, 255, 87);
inline const auto accent2  = juce::Colour::fromRGB(40, 170, 83);
inline const auto edge     = juce::Colour::fromRGB(57, 83, 62);
inline const auto muted    = juce::Colour::fromRGB(139, 170, 145);
inline const auto light    = juce::Colour::fromRGB(224, 241, 226);
inline const auto white    = juce::Colour::fromRGB(244, 255, 244);
inline const auto darkRed  = juce::Colour::fromRGB(32, 72, 42);  // backwards compatible name
inline const auto cyan     = accent;                            // legacy name
inline const auto red      = accent;                            // legacy name
inline juce::Font font(float height, bool bold = false) {
 static auto face=juce::Typeface::createSystemTypefaceFor(BinaryData::TechFont_otf, BinaryData::TechFont_otfSize);
 return juce::Font(juce::FontOptions(face).withHeight(height).withStyle(bold?"Bold":"Regular"));
}
}
