#pragma once
#include "../PluginProcessor.h"
#include "Theme.h"
#include <functional>
#include <vector>

class CurveEditor final : public juce::Component, private juce::Timer {
public:
 explicit CurveEditor(ScratchPadAudioProcessor&);
 void paint(juce::Graphics&) override;
 void resized() override;
 void mouseDown(const juce::MouseEvent&) override;
 void mouseDrag(const juce::MouseEvent&) override;
 void mouseUp(const juce::MouseEvent&) override;
 void mouseMove(const juce::MouseEvent&) override;
 void mouseExit(const juce::MouseEvent&) override;
 void mouseDoubleClick(const juce::MouseEvent&) override;
 void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
 bool keyPressed(const juce::KeyPress&) override;
 void contextMenu(int hitNode = -1);
 void setPreset(const std::vector<CurveNode>& points);
private:
 static constexpr float designW = 291.f, designH = 193.f;
 ScratchPadAudioProcessor& processor;
 juce::Rectangle<float> plot{32.f, 27.f, 250.f, 143.f};
 juce::Image gridCache;
 int selected = -1, hovered = -1, bend = -1, hoveredBend = -1;
 float viewStart = 0.f, viewWidth = 1.f, scrollStart = 0.f;
 float lastPosition = -1.f;
 bool draggingScroll = false, didDrag = false;
 std::vector<std::vector<CurveNode>> undoStack, redoStack;
 juce::Point<float> logical(juce::Point<float> pixel) const;
 juce::Point<float> nodePoint(const CurveNode&) const;
 juce::Point<float> bendPoint(const CurveNode&, const CurveNode&) const;
 float visibleX(float x) const;
 CurveNode fromPoint(juce::Point<float>, bool snap) const;
 int findNode(juce::Point<float>, float radius = 10.f) const;
 int findBend(juce::Point<float>, float radius = 9.f) const;
 void paintGrid(juce::Graphics&);
 void invalidateGrid();
 void timerCallback() override;
 void saveUndo();
 void undo();
 void redo();
 void edit(const std::function<void(std::vector<CurveNode>&)>&, bool customName = true);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurveEditor)
};
