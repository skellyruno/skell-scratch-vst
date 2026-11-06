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
 void mouseMove(const juce::MouseEvent&) override;
 void mouseExit(const juce::MouseEvent&) override;
 void menu();
 void applyPreset(int index);
 std::function<void(int)> loadPreset;
 std::function<void()> beforePresetChange;
private:
 static constexpr float designW = 424.f, designH = 42.f;
 ScratchPadAudioProcessor& processor;
 juce::TextEditor nameEditor;
 int preset=0;
 bool renaming=false;
 int hoveredAction=-1;
 int hitTest(juce::Point<float>) const;
 void rename();
 void finishRename(bool commit);
 void syncPresetIndex();
 juce::Point<float> logical(juce::Point<float>) const;
};
