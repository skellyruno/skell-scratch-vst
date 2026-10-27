#pragma once
#include "../PluginProcessor.h"
#include "Theme.h"

// A plain juce::Component does not provide setTooltip(). The tooltip
// capability comes from JUCE's SettableTooltipClient mixin.
class ScratchKnob final : public juce::Component, public juce::SettableTooltipClient {
public:
 explicit ScratchKnob(ScratchPadAudioProcessor&);
 void paint(juce::Graphics&) override;
 void mouseDown(const juce::MouseEvent&) override;
 void mouseDrag(const juce::MouseEvent&) override;
 void mouseUp(const juce::MouseEvent&) override;
 void mouseDoubleClick(const juce::MouseEvent&) override;
 void mouseEnter(const juce::MouseEvent&) override;
 void mouseExit(const juce::MouseEvent&) override;
private:
 ScratchPadAudioProcessor& processor;
 juce::RangedAudioParameter& parameter;
 std::unique_ptr<juce::ParameterAttachment> attachment;
 float value=100.f,dragValue=100.f, startY=0.f;
 bool active=false,hover=false;
 void change(float);
};
