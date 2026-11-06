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
 constexpr float cx=57.f,cy=52.f,r=45.f;
 g.setColour(Theme::accent.withAlpha(hover?.075f:.028f));
 g.fillEllipse(cx-51.f,cy-51.f,102.f,102.f);
 for(int i=0;i<6;++i){
  const float a=juce::degreesToRadians(-225.f+i*45.f);
  const float b=juce::degreesToRadians(-225.f+(i+1)*45.f);
  juce::Path wedge;
  wedge.addCentredArc(cx,cy,r,r,0,a+.036f,b-.036f,true);
  wedge.lineTo(cx,cy);wedge.closeSubPath();
  // The unlit ring forms a stable, low-contrast backdrop.
  g.setColour(Theme::edge.darker(.24f));
  g.fillPath(wedge);
  const float fraction=juce::jlimit(0.f,1.f,
      (displayValue-i*(100.f/6.f))/(100.f/6.f));
  if(fraction>.001f){
   juce::Path progress;
   progress.addCentredArc(cx,cy,r,r,0,a+.036f,
       a+.036f+(b-a-.072f)*fraction,true);
   progress.lineTo(cx,cy);progress.closeSubPath();
   g.setColour(Theme::accent2.brighter(.08f+i/30.f));
   g.fillPath(progress);
   g.setColour(Theme::accent.withAlpha(.18f));
   g.strokePath(progress,juce::PathStrokeType(.55f));
  }
  // Hairline radial divisions keep the six wedges readable.
  g.setColour(Theme::plot);
  g.drawLine(cx+std::sin(a)*r,cy-std::cos(a)*r,
             cx+std::sin(a)*16.f,cy-std::cos(a)*16.f,.85f);
 }
 // Readable progress marker for fractional values, not only whole wedges.
 const float marker=juce::degreesToRadians(-225.f+270.f*displayValue*.01f);
 const float mx=cx+std::sin(marker)*r*.86f;
 const float my=cy-std::cos(marker)*r*.86f;
 g.setColour(Theme::accent.withAlpha(.28f));
 g.fillEllipse(mx-4.f,my-4.f,8.f,8.f);
 g.setColour(Theme::light);
 g.fillEllipse(mx-1.6f,my-1.6f,3.2f,3.2f);
 g.setColour(Theme::plot);
 g.fillEllipse(cx-29.f,cy-29.f,58.f,58.f);
 g.setColour(Theme::edge.brighter(.18f));
 g.drawEllipse(cx-29.f,cy-29.f,58.f,58.f,1.2f);
 // Original compact Skelly crossed-arrow monogram.
 juce::Path icon;
 icon.startNewSubPath(cx-14,cy-13);icon.lineTo(cx+13,cy+11);icon.lineTo(cx+11,cy+4);
 icon.startNewSubPath(cx+13,cy+11);icon.lineTo(cx+5,cy+12);
 icon.startNewSubPath(cx+14,cy-13);icon.lineTo(cx-13,cy+11);icon.lineTo(cx-11,cy+4);
 icon.startNewSubPath(cx-13,cy+11);icon.lineTo(cx-5,cy+12);
 g.setColour(Theme::accent);
 g.strokePath(icon,juce::PathStrokeType(1.9f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 if (hasKeyboardFocus(true)) {
  g.setColour(Theme::accent.withAlpha(.72f));
  g.drawEllipse(cx-49.f,cy-49.f,98.f,98.f,1.1f);
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
