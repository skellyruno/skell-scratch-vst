#include "ScratchKnob.h"
#include <cmath>

ScratchKnob::ScratchKnob(ScratchPadAudioProcessor& p)
 :processor(p),parameter(*p.apvts.getParameter("scratchAmount"))
{
 attachment=std::make_unique<juce::ParameterAttachment>(parameter,[this](float f){value=f;},nullptr);
 attachment->sendInitialUpdate();
 displayValue=value;
 startTimerHz(60);
 setMouseCursor(juce::MouseCursor::PointingHandCursor);
 setWantsKeyboardFocus(true);
 setTooltip("Scratch Amount: 0% = clean input, 100% = full scratch. Drag, scroll or use arrow keys. Double click = 100%.");
}
void ScratchKnob::paint(juce::Graphics& g)
{
 juce::Graphics::ScopedSaveState saved(g);
 g.addTransform(juce::AffineTransform::scale(getWidth()/114.f,getHeight()/125.f));
 constexpr float cx=57.f,cy=52.f,r=40.f;
 const float start=juce::degreesToRadians(-135.f);
 const float end=start+juce::MathConstants<float>::pi*1.5f;
 const float angle=start+(end-start)*juce::jlimit(0.f,1.f,displayValue*.01f);
 // Skell Spacer-style thin track, single lime progress arc, bone marker.
 juce::Path track,progress;
 track.addCentredArc(cx,cy,r,r,0.f,start,end,true);
 progress.addCentredArc(cx,cy,r,r,0.f,start,angle,true);
 g.setColour(Theme::edge);
 g.strokePath(track,juce::PathStrokeType(5.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 if(displayValue>.1f) {
  g.setColour(Theme::accent);
  g.strokePath(progress,juce::PathStrokeType(4.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 }
 g.setColour(Theme::elevated);
 g.fillEllipse(cx-32.f,cy-32.f,64.f,64.f);
 g.setColour(Theme::edge);
 g.drawEllipse(cx-32.f,cy-32.f,64.f,64.f,1.f);
 // The original crossed-arrow Scratch identity remains inside the dial.
 juce::Path icon;
 icon.startNewSubPath(cx-12,cy-11);icon.lineTo(cx+11,cy+10);icon.lineTo(cx+9,cy+4);
 icon.startNewSubPath(cx+11,cy+10);icon.lineTo(cx+4,cy+11);
 icon.startNewSubPath(cx+12,cy-11);icon.lineTo(cx-11,cy+10);icon.lineTo(cx-9,cy+4);
 icon.startNewSubPath(cx-11,cy+10);icon.lineTo(cx-4,cy+11);
 g.setColour(Theme::accent);
 g.strokePath(icon,juce::PathStrokeType(1.7f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 const auto marker=juce::Point<float>(cx+r*std::sin(angle),cy-r*std::cos(angle));
 g.setColour(Theme::light);
 g.fillRect(juce::Rectangle<float>(5.f,5.f).withCentre(marker));
 if(hasKeyboardFocus(true)) {
  g.setColour(Theme::accent.withAlpha(.7f));
  g.drawEllipse(cx-45.f,cy-45.f,90.f,90.f,1.f);
 }
 g.setColour(Theme::light);
 g.setFont(Theme::uiFont(11.f,true));
 g.drawText("SCRATCH",0,101,114,13,juce::Justification::centred);
 g.setColour(Theme::accent);
 g.setFont(Theme::uiFont(10.5f,true));
 g.drawText(juce::String(juce::roundToInt(displayValue))+"%",0,113,114,12,juce::Justification::centred);
}
void ScratchKnob::change(float newValue) { attachment->setValueAsPartOfGesture(juce::jlimit(0.f,100.f,newValue)); }
void ScratchKnob::mouseDown(const juce::MouseEvent& e)
{
 if(e.getNumberOfClicks()>1)return;
 grabKeyboardFocus();
 startY=(float)e.y;dragValue=value;active=true;attachment->beginGesture();
}
void ScratchKnob::mouseDrag(const juce::MouseEvent& e)
{
 if(active)change(dragValue+(startY-e.y)*.80f*125.f/std::max(1,getHeight()));
}
void ScratchKnob::mouseUp(const juce::MouseEvent&)
{
 if(active){attachment->endGesture();active=false;}
}
void ScratchKnob::mouseDoubleClick(const juce::MouseEvent&){attachment->setValueAsCompleteGesture(100.f);}
void ScratchKnob::mouseEnter(const juce::MouseEvent&){hover=true;repaint();}
void ScratchKnob::mouseExit(const juce::MouseEvent&){hover=false;repaint();}

void ScratchKnob::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
 if(wheel.deltaY==0.f) return;
 attachment->setValueAsCompleteGesture(juce::jlimit(0.f,100.f,
     value + (wheel.deltaY>0.f?1.f:-1.f) * 2.f));
}
bool ScratchKnob::keyPressed(const juce::KeyPress& key)
{
 const int code=key.getKeyCode();
 const float increment=key.getModifiers().isShiftDown()?.1f:1.f;
 if(code==juce::KeyPress::upKey||code==juce::KeyPress::rightKey){
  attachment->setValueAsCompleteGesture(juce::jlimit(0.f,100.f,value+increment));return true;
 }
 if(code==juce::KeyPress::downKey||code==juce::KeyPress::leftKey){
  attachment->setValueAsCompleteGesture(juce::jlimit(0.f,100.f,value-increment));return true;
 }
 if(code==juce::KeyPress::homeKey){attachment->setValueAsCompleteGesture(0.f);return true;}
 if(code==juce::KeyPress::endKey){attachment->setValueAsCompleteGesture(100.f);return true;}
 return false;
}

void ScratchKnob::timerCallback()
{
 const float difference=value-displayValue;
 if (std::abs(difference)>.045f) {
  displayValue+=difference*.28f;
  repaint();
 } else if (displayValue!=value) {
  displayValue=value;
  repaint();
 }
}
