#pragma once
#include "Theme.h"

// A toggled APVTS-backed button. Display remains STOP in either state so
// automation/armed state is not mistaken for a different command.
class StopButton final : public juce::TextButton {
public:
 StopButton() : juce::TextButton("STOP") {
  setMouseCursor(juce::MouseCursor::PointingHandCursor);
  setClickingTogglesState(false);
  setTooltip("STOP is automatable. When engaged it fades the output to silence; click again to unmute.");
 }
 void paintButton(juce::Graphics& g, bool hovered, bool pressed) override {
  juce::Graphics::ScopedSaveState state(g);
  g.addTransform(juce::AffineTransform::scale(getWidth()/107.f,getHeight()/38.f));
  const auto r=juce::Rectangle<float>(.5f,.5f,106.f,37.f);
  const bool halted=getToggleState();
  g.setColour(pressed?Theme::elevated:halted?Theme::elevated:Theme::panel);
  g.fillRect(r);
  g.setColour(halted?Theme::accent:Theme::edge);
  g.drawRect(r,halted?1.2f:1.f);
  g.setColour(halted?Theme::accent:Theme::light);
  g.setFont(Theme::uiFont(12.f,true));
  g.drawText("STOP",10,5,78,17,juce::Justification::centredLeft);
  g.setColour(halted?Theme::accent:Theme::muted);
  g.setFont(Theme::uiFont(8.f));
  g.drawText(halted?"MUTED":"MUTE OUTPUT",10,23,87,11,juce::Justification::centredLeft);
  g.setColour(halted?Theme::accent:hovered?Theme::light:Theme::faint);
  g.fillRect(juce::Rectangle<float>(94.f,14.f,5.f,5.f));
  if(hasKeyboardFocus(true)){
   g.setColour(Theme::accent);
   g.drawRect(r.reduced(1.5f),1.f);
  }
 }

};
