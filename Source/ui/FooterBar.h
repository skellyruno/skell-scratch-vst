#pragma once
#include "../PluginProcessor.h"
#include "Theme.h"
class FooterBar:public juce::Component {
public:
 explicit FooterBar(ScratchPadAudioProcessor&);
 void paint(juce::Graphics&)override;
 void mouseUp(const juce::MouseEvent&)override;
 void menu();
 std::function<void(int)> loadPreset;
private:
 ScratchPadAudioProcessor& processor;
 int preset=0;
 void rename();
};
