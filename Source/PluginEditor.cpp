#include "PluginEditor.h"

ScratchPadAudioProcessorEditor::ScratchPadAudioProcessorEditor(ScratchPadAudioProcessor& p)
 : AudioProcessorEditor(p), processor(p), curve(p), knob(p), footer(p), tooltipWindow(this, 750)
{
 setOpaque(true);
 setSize(441,263);
 setResizable(true,true);
 constrainer.setFixedAspectRatio(441.0/263.0);
 constrainer.setSizeLimits(441,263,1323,789);
 setConstrainer(&constrainer);
 addAndMakeVisible(curve);
 addAndMakeVisible(knob);
 addAndMakeVisible(footer);
 addAndMakeVisible(stop);
 stopAttachment = std::make_unique<juce::ParameterAttachment>(
  *processor.apvts.getParameter("stopTrigger"),
  [this](float amount) {
   stop.setToggleState(amount>=.5f,juce::dontSendNotification);
   stop.repaint();
  },nullptr);
 stop.onClick=[this] {
  const auto* parameter=processor.apvts.getRawParameterValue("stopTrigger");
  stopAttachment->setValueAsCompleteGesture(parameter->load()>=.5f?0.f:1.f);
 };
 stopAttachment->sendInitialUpdate();
 footer.loadPreset=[this](int){curve.repaint();}; // FooterBar owns preset application.
 startTimerHz(15);
}
void ScratchPadAudioProcessorEditor::paint(juce::Graphics& g)
{
 g.fillAll(Theme::bg);
 juce::Graphics::ScopedSaveState saved(g);
 g.addTransform(juce::AffineTransform::scale(getWidth()/441.f,getHeight()/263.f));
 // Thin, double-chamfered neon shell; left/right edges are design-time vector art.
 juce::Path outer;
 outer.startNewSubPath(9,3);outer.lineTo(432,3);
 outer.lineTo(438,9);outer.lineTo(438,254);outer.lineTo(432,260);
 outer.lineTo(9,260);outer.lineTo(3,254);outer.lineTo(3,9);outer.closeSubPath();
 g.setColour(Theme::accent.withAlpha(.16f));g.strokePath(outer,juce::PathStrokeType(4.0f));
 g.setColour(Theme::accent.withAlpha(.95f));g.strokePath(outer,juce::PathStrokeType(1.2f));
 g.setColour(Theme::edge.withAlpha(.80f));g.drawRoundedRectangle(7.f,7.f,427.f,248.f,5.f,.75f);
 g.setColour(Theme::accent.withAlpha(.35f));g.drawLine(9.f,209.f,432.f,209.f,1.f);
 // Small registration ticks make the visual frame intentional at every scale.
 g.setColour(Theme::accent.withAlpha(.56f));
 for(int i=0;i<3;++i){float x=15.f+i*8.f;g.drawLine(x,204.f,x+3.f,204.f,.8f);}
}
void ScratchPadAudioProcessorEditor::resized()
{
 const auto bounds=[this](int x,int y,int w,int h){
  return juce::Rectangle<int>(juce::roundToInt(x*getWidth()/441.f),juce::roundToInt(y*getHeight()/263.f),
                              juce::roundToInt(w*getWidth()/441.f),juce::roundToInt(h*getHeight()/263.f));
 };
 curve.setBounds(bounds(14,8,291,193));
 knob.setBounds(bounds(314,9,114,125));
 stop.setBounds(bounds(318,144,107,49));
 footer.setBounds(bounds(9,215,424,42));
}
void ScratchPadAudioProcessorEditor::timerCallback() { footer.repaint(); }
