#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>
namespace Theme {
inline const auto bg=juce::Colour::fromString("ff1b1d20");
inline const auto panel=juce::Colour::fromString("ff2b2c2e");
inline const auto cyan=juce::Colour::fromString("ff2ec4d6");
inline const auto red=juce::Colour::fromString("ffc0141c");
inline const auto darkRed=juce::Colour::fromString("ff7a0f14");
inline const auto light=juce::Colour::fromString("ffd0d0d0");
inline juce::Font font(float height, bool bold=false) {
 static auto face=juce::Typeface::createSystemTypefaceFor(BinaryData::TechFont_otf, BinaryData::TechFont_otfSize);
 return juce::Font(juce::FontOptions(face).withHeight(height).withStyle(bold ? "Bold" : "Regular"));
}
}
