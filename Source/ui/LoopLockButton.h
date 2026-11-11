#pragma once
#include "../PluginProcessor.h"
#include "Theme.h"

// Compact automatable capture control. No new floating UI or heavyweight
// graphics dependencies: the existing editor still uses a 441 x 263 layout.
class LoopLockButton final : public juce::TextButton {
public:
 explicit LoopLockButton(ScratchPadAudioProcessor& p)
     : juce::TextButton("LOOP LOCK"), processor(p) {
  setClickingTogglesState(false);
  setMouseCursor(juce::MouseCursor::PointingHandCursor);
  setTooltip("Capture the next complete input loop, then scratch it indefinitely. Click again for live mode. Automatable.");
 }
 void paintButton(juce::Graphics& g, bool over, bool down) override {
  juce::Graphics::ScopedSaveState saved(g);
  g.addTransform(juce::AffineTransform::scale(getWidth()/107.f,getHeight()/25.f));
  const bool enabled=getToggleState();
  const int stage=processor.lockedStatus.load(std::memory_order_relaxed);
  const float progress=juce::jlimit(0.f,1.f,processor.lockedProgress.load(std::memory_order_relaxed));
  const auto rect=juce::Rectangle<float>(1.f,1.f,105.f,23.f);
  g.setColour(enabled?Theme::elevated:Theme::panel);
  g.fillRect(rect);
  g.setColour(enabled?Theme::accent:Theme::edge.withAlpha(over?.95f:.75f));
  g.drawRect(rect,1.f);
  if(enabled && stage==1){
   g.setColour(Theme::accent.withAlpha(.12f));
   g.fillRect(juce::Rectangle<float>(3.f,21.f,101.f*progress,1.4f));
  }
  if(down){g.setColour(Theme::accent.withAlpha(.08f));g.fillRect(rect.reduced(1.f));}
  g.setFont(Theme::uiFont(9.6f,true));
  g.setColour(enabled?Theme::accent:Theme::light);
  g.drawText("LOOP LOCK",7,4,66,17,juce::Justification::centredLeft);
  g.setFont(Theme::uiFont(7.7f,true));
  g.setColour(stage==2?Theme::accent:Theme::muted);
  const juce::String status=stage==2?"HELD":stage==1?(juce::String(juce::roundToInt(progress*100.f))+"%"):enabled?"ARM":"LIVE";
  g.drawText(status,68,4,30,17,juce::Justification::centredRight);
  g.setColour(stage==2?Theme::accent:stage==1?Theme::accent2:Theme::muted);
  g.fillRect(juce::Rectangle<float>(98.f,11.f,3.f,3.f));
  if(hasKeyboardFocus(true)) {
   g.setColour(Theme::accent.withAlpha(.85f));
   g.drawRect(rect.reduced(.8f),1.15f);
  }
 }
private:
 ScratchPadAudioProcessor& processor;
};
