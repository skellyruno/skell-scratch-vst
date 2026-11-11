#pragma once
#include <JuceHeader.h>

// SKELL house "Bone" style, cross-checked with skell-key-vst and
// skell-spacer-vst. Warm blacks, ivory type, one lime accent, no outer glow.
namespace Theme {
inline const juce::Colour bg       { 0xff0b0a09 };
inline const juce::Colour panel    { 0xff131210 };
inline const juce::Colour elevated { 0xff1b1915 };
inline const juce::Colour plot     { 0xff0b0a09 };
inline const juce::Colour accent   { 0xffb6f56a };
inline const juce::Colour accent2  { 0xff85cb4c };
inline const juce::Colour edge     { 0xff2f2b23 };
inline const juce::Colour muted    { 0xff9b937d };
inline const juce::Colour faint    { 0xff4d483c };
inline const juce::Colour light    { 0xffe9e1cc };
inline const juce::Colour white    { 0xfffaf6e9 };

inline juce::Font font(float height, bool bold = false) {
 juce::Font f(juce::FontOptions(height, bold ? juce::Font::bold : juce::Font::plain));
 return f.withExtraKerningFactor(bold ? 0.04f : 0.015f);
}
inline juce::Font uiFont(float height, bool bold = false) {
 return juce::Font(juce::FontOptions(height, bold ? juce::Font::bold : juce::Font::plain));
}

// Corner-bracket card geometry is shared with Skell Spacer's Bone theme.
inline void drawCard(juce::Graphics& g, juce::Rectangle<float> r) {
 g.setColour(panel);g.fillRect(r);
 g.setColour(edge);g.drawRect(r,1.f);
 constexpr float length=8.f;
 const auto a=r.getTopLeft(),b=r.getTopRight();
 const auto c=r.getBottomLeft(),d=r.getBottomRight();
 juce::Path corners;
 corners.startNewSubPath(a.x,a.y+length);corners.lineTo(a.x,a.y);corners.lineTo(a.x+length,a.y);
 corners.startNewSubPath(b.x-length,b.y);corners.lineTo(b.x,b.y);corners.lineTo(b.x,b.y+length);
 corners.startNewSubPath(c.x,c.y-length);corners.lineTo(c.x,c.y);corners.lineTo(c.x+length,c.y);
 corners.startNewSubPath(d.x-length,d.y);corners.lineTo(d.x,d.y);corners.lineTo(d.x,d.y-length);
 g.setColour(muted.withAlpha(.7f));g.strokePath(corners,juce::PathStrokeType(1.15f));
}

// Matches the skull proportions and facial cut-outs of Skell Key/Spacer.
// Small enough to serve as a real vector brand mark at the bottom of Pad.
inline void drawSkull(juce::Graphics& g, float x, float y, float scale = 1.f) {
 juce::Graphics::ScopedSaveState saved(g);
 g.addTransform(juce::AffineTransform::scale(scale,scale).translated(x,y));
 g.setColour(light);
 g.fillEllipse(1.f,1.f,25.f,23.f);
 g.fillRoundedRectangle(6.5f,18.f,14.f,10.f,3.f);
 g.setColour(panel);
 g.fillEllipse(6.2f,9.4f,6.4f,6.8f);
 g.fillEllipse(14.6f,9.4f,6.4f,6.8f);
 juce::Path nose;nose.addTriangle(13.5f,16.f,11.6f,20.f,15.4f,20.f);g.fillPath(nose);
 for(int i=0;i<3;++i)g.fillRect(9.6f+3.4f*i,23.f,1.f,5.f);
}
}
