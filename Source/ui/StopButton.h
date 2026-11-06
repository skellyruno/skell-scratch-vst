#pragma once
#include "Theme.h"

// A toggled APVTS-backed button. Display remains STOP in either state so
// automation/armed state is not mistaken for a different command.
class StopButton final : public juce::TextButton {
public:
 StopButton() : juce::TextButton("STOP") {
  setMouseCursor(juce::MouseCursor::PointingHandCursor);
  setClickingTogglesState(false);
  setTooltip("Automatable STOP: click to toggle. 100% = halted; 0% = running.");
 }
 void paintButton(juce::Graphics& g, bool hovered, bool pressed) override {
  const auto sx = getWidth() / 107.f, sy = getHeight() / 49.f;
  juce::Graphics::ScopedSaveState state(g);
  g.addTransform(juce::AffineTransform::scale(sx, sy));
  const auto r=juce::Rectangle<float>(1.5f,1.5f,104.f,46.f);
  const bool halted=getToggleState();
  const auto top=halted?juce::Colour::fromRGB(29,73,47):Theme::elevated;
  const auto bottom=halted?juce::Colour::fromRGB(22,52,35):Theme::panel;
  g.setColour(juce::Colours::black.withAlpha(.22f));
  g.fillRoundedRectangle(r.translated(0.f,1.f),7.f);
  g.setGradientFill(juce::ColourGradient(top.brighter(hovered?.13f:0.f),r.getTopLeft(),bottom,r.getBottomRight(),false));
  g.fillRoundedRectangle(r,7.f);
  g.setColour(Theme::accent.withAlpha(halted?.78f:hovered?.65f:.28f));
  g.drawRoundedRectangle(r,7.f,halted?1.5f:1.f);
  if(pressed){g.setColour(Theme::accent.withAlpha(.10f));g.fillRoundedRectangle(r.reduced(1.f),6.f);}
  g.setColour(halted?Theme::accent:Theme::light);
  g.setFont(Theme::uiFont(14.f,true));
  g.drawText("STOP",8,11,82,18,juce::Justification::centred);
  g.setColour(halted?Theme::accent.withAlpha(.87f):Theme::muted);
  g.setFont(Theme::uiFont(8.3f,true));
  g.drawText(halted?"ENGAGED":"CLICK TO HALT",8,28,82,12,juce::Justification::centred);
  g.setColour(halted?Theme::accent:Theme::edge);
  g.fillEllipse(91.f,21.f,5.f,5.f);
  if(halted){g.setColour(Theme::accent.withAlpha(.22f));g.drawEllipse(88.5f,18.5f,10.f,10.f,1.f);}
 }
};
