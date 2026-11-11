#pragma once
#include "PluginProcessor.h"
#include "ui/CurveEditor.h"
#include "ui/ScratchKnob.h"
#include "ui/FooterBar.h"
#include "ui/StopButton.h"
#include "ui/LoopLockButton.h"
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
 LoopLockButton lock;
 juce::ComponentBoundsConstrainer constrainer;
 // Owns hover tooltips for both ScratchKnob and StopButton.
 juce::TooltipWindow tooltipWindow;
 std::unique_ptr<juce::ParameterAttachment> stopAttachment;
 std::unique_ptr<juce::ParameterAttachment> lockAttachment;
 float meterLevel=0.f;
 int previousMeterSegments=-1;
 int previousLockState=-1;
 float previousLockProgress=-1.f;
 juce::String lastPresetName;
 void timerCallback()override;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScratchPadAudioProcessorEditor)
};
