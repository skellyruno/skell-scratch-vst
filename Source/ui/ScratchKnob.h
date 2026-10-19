#pragma once
#include "../PluginProcessor.h"
#include "Theme.h"
class ScratchKnob:public juce::Component {
public:
 explicit ScratchKnob(ScratchPadAudioProcessor&);
 void paint(juce::Graphics&)override;
 void mouseDown(const juce::MouseEvent&)override;
 void mouseDrag(const juce::MouseEvent&)override;
 void mouseUp(const juce::MouseEvent&)override;
 void mouseDoubleClick(const juce::MouseEvent&)override;
private:
 ScratchPadAudioProcessor& processor;
 juce::RangedAudioParameter& parameter;
 std::unique_ptr<juce::ParameterAttachment> attachment;
 float value=100,dragValue=100;
 int startY=0;
 void change(float);
};
