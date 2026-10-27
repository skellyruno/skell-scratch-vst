#pragma once
#include "../PluginProcessor.h"
#include "Theme.h"
#include <functional>

class FooterBar final : public juce::Component {
public:
 explicit FooterBar(ScratchPadAudioProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
 void mouseUp(const juce::MouseEvent&) override;
 void menu();
 void applyPreset(int index);
 std::function<void(int)> loadPreset;
private:
 static constexpr float designW = 424.f, designH = 42.f;
 ScratchPadAudioProcessor& processor;
 juce::TextEditor nameEditor;
 int preset=0;
 bool renaming=false;
 void rename();
 void finishRename(bool commit);
 juce::Point<float> logical(juce::Point<float>) const;
};
