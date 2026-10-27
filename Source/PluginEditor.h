#pragma once
#include "PluginProcessor.h"
#include "ui/CurveEditor.h"
#include "ui/ScratchKnob.h"
#include "ui/FooterBar.h"
#include "ui/StopButton.h"
class ScratchPadAudioProcessorEditor:public juce::AudioProcessorEditor,private juce::Timer{
public:
 explicit ScratchPadAudioProcessorEditor(ScratchPadAudioProcessor&);
 void paint(juce::Graphics&)override;
 void resized()override;
private:
 ScratchPadAudioProcessor& processor;
 CurveEditor curve;
 ScratchKnob knob;
 FooterBar footer;
 StopButton stop;
 juce::ComponentBoundsConstrainer constrainer;
 // Owns hover tooltips for both ScratchKnob and StopButton.
 juce::TooltipWindow tooltipWindow;
 std::unique_ptr<juce::ParameterAttachment> stopAttachment;
 void timerCallback()override;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScratchPadAudioProcessorEditor)
};
