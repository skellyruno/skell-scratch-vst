#pragma once
#include "Theme.h"

class StopButton final : public juce::TextButton {
public:
 StopButton() : juce::TextButton("STOP") {
  setMouseCursor(juce::MouseCursor::PointingHandCursor);
  setClickingTogglesState(false);
  setTooltip("DAW-automatable: click to toggle Stop. 1 = stopped; 0 = playing.");
 }
 void paintButton(juce::Graphics& g, bool isHovered, bool isPressed) override {
  auto bounds=getLocalBounds().toFloat().reduced(1.5f);
  const bool stopped=getToggleState();
  const auto base=stopped?Theme::darkRed:Theme::elevated;
  g.setColour(juce::Colours::black.withAlpha(.35f));
  g.fillRoundedRectangle(bounds.translated(0.f,2.f),8.f);
  g.setGradientFill(juce::ColourGradient(
   base.brighter(isHovered?.23f:.10f),bounds.getTopLeft(),
   base.darker(isPressed?.08f:.26f),bounds.getBottomRight(),false));
  g.fillRoundedRectangle(bounds,8.f);
  g.setColour(Theme::accent.withAlpha(stopped?1.f:isHovered?.88f:.38f));
  g.drawRoundedRectangle(bounds.reduced(.5f),8.f,stopped?1.6f:1.f);
  g.setColour(Theme::accent.withAlpha(stopped?.36f:.14f));
  g.drawLine(bounds.getX()+9.f,bounds.getY()+6.f,bounds.getRight()-9.f,bounds.getY()+6.f,1.f);
  const float h=std::max(11.f,bounds.getHeight()*.25f);
  g.setFont(Theme::font(h,true));
  g.setColour(stopped?Theme::accent:Theme::light);
  g.drawFittedText(stopped?"STOPPED":"STOP",bounds.toNearestInt(),juce::Justification::centred,1);
  if(stopped){g.setColour(Theme::accent);g.fillEllipse(bounds.getRight()-12.f,bounds.getCentreY()-2.f,4.f,4.f);}
 }
};
