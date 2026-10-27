#include "ScratchKnob.h"
#include <cmath>

ScratchKnob::ScratchKnob(ScratchPadAudioProcessor& p)
 :processor(p),parameter(*p.apvts.getParameter("scratchAmount"))
{
 attachment=std::make_unique<juce::ParameterAttachment>(parameter,[this](float f){value=f;repaint();},nullptr);
 attachment->sendInitialUpdate();
 setMouseCursor(juce::MouseCursor::PointingHandCursor);
 setTooltip("Scratch depth. Drag up/down; double-click to reset to 100%.");
}
void ScratchKnob::paint(juce::Graphics& g)
{
 juce::Graphics::ScopedSaveState saved(g);
 g.addTransform(juce::AffineTransform::scale(getWidth()/114.f,getHeight()/125.f));
 constexpr float cx=57.f,cy=52.f,r=45.f;
 g.setColour(Theme::accent.withAlpha(hover?.10f:.05f));
 g.fillEllipse(cx-51.f,cy-51.f,102.f,102.f);
 for(int i=0;i<6;++i){
  const float a=juce::degreesToRadians(-225.f+i*45.f);
  const float b=juce::degreesToRadians(-225.f+(i+1)*45.f);
  juce::Path wedge;
  wedge.addCentredArc(cx,cy,r,r,0,a+.036f,b-.036f,true);
  wedge.lineTo(cx,cy);wedge.closeSubPath();
  const bool lit=value>(i+0.1f)*(100.f/6.f);
  g.setColour(lit?Theme::accent2.brighter(i/16.f):Theme::edge.darker(.45f));
  g.fillPath(wedge);
  g.setColour(Theme::accent.withAlpha(lit?.30f:.12f));
  g.strokePath(wedge,juce::PathStrokeType(.6f));
  // JUCE arc angle uses x=sin(angle), y=-cos(angle).
  g.setColour(Theme::plot);
  g.drawLine(cx+std::sin(a)*r,cy-std::cos(a)*r,cx+std::sin(a)*16.f,cy-std::cos(a)*16.f,.85f);
 }
 g.setColour(Theme::plot);
 g.fillEllipse(cx-29.f,cy-29.f,58.f,58.f);
 g.setColour(Theme::accent.withAlpha(.6f));
 g.drawEllipse(cx-29.f,cy-29.f,58.f,58.f,1.2f);
 // Original compact Skelly crossed-arrow monogram.
 juce::Path icon;
 icon.startNewSubPath(cx-14,cy-13);icon.lineTo(cx+13,cy+11);icon.lineTo(cx+11,cy+4);
 icon.startNewSubPath(cx+13,cy+11);icon.lineTo(cx+5,cy+12);
 icon.startNewSubPath(cx+14,cy-13);icon.lineTo(cx-13,cy+11);icon.lineTo(cx-11,cy+4);
 icon.startNewSubPath(cx-13,cy+11);icon.lineTo(cx-5,cy+12);
 g.setColour(Theme::accent);
 g.strokePath(icon,juce::PathStrokeType(1.9f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 g.setColour(Theme::light);
 g.setFont(Theme::font(12.f,true));
 g.drawText("SCRATCH",0,100,114,13,juce::Justification::centred);
 g.setColour(Theme::muted);
 g.setFont(Theme::font(9.f));
 g.drawText(juce::String(juce::roundToInt(value))+"%",0,113,114,10,juce::Justification::centred);
}
void ScratchKnob::change(float newValue) { attachment->setValueAsPartOfGesture(juce::jlimit(0.f,100.f,newValue)); }
void ScratchKnob::mouseDown(const juce::MouseEvent& e)
{
 if(e.getNumberOfClicks()>1)return;
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
