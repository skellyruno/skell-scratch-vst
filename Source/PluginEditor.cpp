#include "PluginEditor.h"
#include <cmath>

ScratchPadAudioProcessorEditor::ScratchPadAudioProcessorEditor(ScratchPadAudioProcessor& p)
 : AudioProcessorEditor(p), processor(p), curve(p), knob(p), footer(p), lock(p), tooltipWindow(this, 750)
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
 addAndMakeVisible(lock);
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
 lockAttachment=std::make_unique<juce::ParameterAttachment>(
  *processor.apvts.getParameter("loopLock"),
  [this](float value) {
   lock.setToggleState(value>=.5f,juce::dontSendNotification);
   lock.repaint();
  },nullptr);
 lock.onClick=[this] {
  const auto* param=processor.apvts.getRawParameterValue("loopLock");
  lockAttachment->setValueAsCompleteGesture(param->load()>=.5f?0.f:1.f);
 };
 lockAttachment->sendInitialUpdate();
 footer.beforePresetChange=[this]{curve.rememberExternalChange();};
 footer.loadPreset=[this](int){curve.resetSelectionAndRepaint();};
 footer.requestTutorial=[this]{showGuidedTour(false);};
 guidedTour=std::make_unique<skellcore::ui::GuidedTourController>(*this,"skell-pad",1);
 skellcore::ui::GuidedTourPalette colours;
 colours.accent=Theme::accent;
 colours.panel=Theme::panel;
 colours.text=Theme::light;
 colours.secondary=Theme::muted;
 colours.shade=juce::Colours::black.withAlpha(.78f);
 guidedTour->setPalette(colours);
 guidedTour->setSteps({
  {&curve,"DRAW YOUR SCRATCH","Click on the curve to add a dot. Drag dots for speed; drag small diamonds to bend. Alt disables snapping."},
  {&knob,"SCRATCH AMOUNT","Turn this dial to blend the effect. 0% is clean audio; 100% is full scratching."},
  {&lock,"LOOP LOCK","Capture one full loop, then keep scratching the same sound. HELD means the recording is ready. Tap again for live audio."},
  {&stop,"STOP / UNMUTE","Stop mutes the audio output; it does not delete your loop. Click again to hear it."},
  {&footer,"PRESETS & MENU","Use the preset arrows or open MENU for loop length, sync and presets. MENU > Guided tutorial replays this tour."}
 });
 startTimerHz(30);
}
void ScratchPadAudioProcessorEditor::showGuidedTour(bool firstRun)
{
 if(!guidedTour)return;
 if(firstRun)guidedTour->startIfFirstRun();
 else guidedTour->replay();
}
bool ScratchPadAudioProcessorEditor::showGuidedTourForScreenshot()
{
 if(!guidedTour)return false;
 return guidedTour->replay();
}
skellcore::ui::GuidedTourOverlay* ScratchPadAudioProcessorEditor::guidedTourOverlayForScreenshot() const noexcept
{
 return guidedTour ? guidedTour->overlay() : nullptr;
}
void ScratchPadAudioProcessorEditor::dismissGuidedTourForScreenshot()
{
 if(guidedTour) guidedTour->dismiss();
}
void ScratchPadAudioProcessorEditor::paint(juce::Graphics& g)
{
 g.fillAll(Theme::bg);
 juce::Graphics::ScopedSaveState saved(g);
 g.addTransform(juce::AffineTransform::scale(getWidth()/441.f,getHeight()/263.f));
 // The other Skell plugins use a quiet warm-black field with fine border
 // and bone corner brackets, not an outlined neon-green hexagonal shell.
 g.setColour(Theme::edge);
 g.drawRect(juce::Rectangle<float>(4.f,4.f,433.f,255.f),.9f);
 const float left=4.f,right=437.f,top=4.f,bottom=259.f;
 juce::Path brackets;
 brackets.startNewSubPath(left,top+10);brackets.lineTo(left,top);brackets.lineTo(left+10,top);
 brackets.startNewSubPath(right-10,top);brackets.lineTo(right,top);brackets.lineTo(right,top+10);
 brackets.startNewSubPath(left,bottom-10);brackets.lineTo(left,bottom);brackets.lineTo(left+10,bottom);
 brackets.startNewSubPath(right-10,bottom);brackets.lineTo(right,bottom);brackets.lineTo(right,bottom-10);
 g.setColour(Theme::muted.withAlpha(.50f));
 g.strokePath(brackets,juce::PathStrokeType(1.15f));
 // Skell Key-style 48-tick audio activity strip, anchored just above the
 // footer rather than adding another panel. Brightness follows actual output
 // signal peak from processBlock, never the clock or an invented animation.
 constexpr int ticks=48;
 constexpr float startX=14.f, span=413.f, pitch=span/ticks;
 const int active=juce::jlimit(0,ticks,juce::roundToInt(meterLevel*ticks));
 for (int i=0;i<ticks;++i) {
  const bool lit=i<active;
  const float height=lit?5.6f:2.0f;
  const float y=210.5f-height*.5f;
  g.setColour(lit?Theme::accent.withAlpha(.48f+.48f*i/ticks)
                 :Theme::faint.withAlpha(.58f));
  g.fillRoundedRectangle(startX+i*pitch,y,pitch-2.2f,height,.8f);
 }

}
void ScratchPadAudioProcessorEditor::resized()
{
 const auto bounds=[this](int x,int y,int w,int h){
  return juce::Rectangle<int>(juce::roundToInt(x*getWidth()/441.f),juce::roundToInt(y*getHeight()/263.f),
                              juce::roundToInt(w*getWidth()/441.f),juce::roundToInt(h*getHeight()/263.f));
 };
 curve.setBounds(bounds(14,8,291,193));
 knob.setBounds(bounds(314,9,114,125));
 stop.setBounds(bounds(318,140,107,38));
 lock.setBounds(bounds(318,181,107,25));
 footer.setBounds(bounds(9,215,424,42));
}
void ScratchPadAudioProcessorEditor::timerCallback()
{
 // Only show the first-run overlay when the host actually displays the
 // editor. Headless screenshots must retain an unobscured default view.
 if(!firstRunTourChecked && isShowing() && getPeer()!=nullptr){
  firstRunTourChecked=true;
  showGuidedTour(true);
 }
 processor.collectRetiredCurves();
 const float peak=ScratchDSP::consumePeak(processor.outputPeakHold);
 const float nextMeter=ScratchDSP::meterFollow(meterLevel,ScratchDSP::peakToDisplay(peak));
 const int nextSegments=juce::jlimit(0,48,juce::roundToInt(nextMeter*48.f));
 if(nextSegments!=previousMeterSegments){
  previousMeterSegments=nextSegments;
  repaint(juce::Rectangle<int>(0,juce::roundToInt(getHeight()*204.f/263.f),
                              getWidth(),juce::roundToInt(getHeight()*11.f/263.f)+2));
 }
 meterLevel=nextMeter;
 const int state=processor.lockedStatus.load(std::memory_order_relaxed);
 const float progress=processor.lockedProgress.load(std::memory_order_relaxed);
 if(state!=previousLockState || std::abs(progress-previousLockProgress)>.015f){
  previousLockState=state;previousLockProgress=progress;lock.repaint();
 }
 const auto name=processor.getPresetName();
 if(lastPresetName != name){
  lastPresetName=name;
  footer.refreshPresetState();
 }
}
