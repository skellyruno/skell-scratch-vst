#pragma once
#include "../PluginProcessor.h"
#include "Theme.h"
class CurveEditor : public juce::Component,private juce::Timer {
public:
 explicit CurveEditor(ScratchPadAudioProcessor& p);
 void paint(juce::Graphics&) override;
 void resized() override;
 void mouseDown(const juce::MouseEvent&) override;
 void mouseDrag(const juce::MouseEvent&) override;
 void mouseUp(const juce::MouseEvent&) override;
 void mouseDoubleClick(const juce::MouseEvent&) override;
 void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails&) override;
 void contextMenu();
 void setPreset(const std::vector<CurveNode>& n){processor.setCurve(n);repaint();}
private:
 ScratchPadAudioProcessor& processor;
 juce::Rectangle<float> plot;
 int dragged=-1,bend=-1;
 float viewStart=0,viewWidth=1,scrollStart=0;
 float lastPosition=0;
 bool draggingScroll=false;
 juce::Point<float> point(const CurveNode&) const;
 float visibleX(float)const;
 CurveNode fromPoint(juce::Point<float>,bool)const;
 void timerCallback()override;
 void edit(const std::function<void(std::vector<CurveNode>&)>&);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurveEditor)
};
