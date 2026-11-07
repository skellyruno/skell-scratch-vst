#include "CurveEditor.h"
#include "PresetBank.h"
#include "../LoopTiming.h"
#include <algorithm>
#include <cmath>

CurveEditor::CurveEditor(ScratchPadAudioProcessor& p) : processor(p)
{
 setOpaque(true);
 setMouseCursor(juce::MouseCursor::CrosshairCursor);
 setTooltip("Click to add points; drag circles or diamond bend handles. Snap = 1/16 beat, Alt = free, Ctrl+wheel = zoom, Ctrl+Z = undo.");
 setWantsKeyboardFocus(true);
 startTimerHz(60);
}
void CurveEditor::resized() { invalidateGrid(); }
juce::Point<float> CurveEditor::logical(juce::Point<float> p) const
{
 return {p.x * designW / std::max(1, getWidth()), p.y * designH / std::max(1, getHeight())};
}
float CurveEditor::visibleX(float x) const { return plot.getX() + (x - viewStart) * plot.getWidth() / viewWidth; }
juce::Point<float> CurveEditor::nodePoint(const CurveNode& n) const
{
 return {visibleX(n.x), plot.getCentreY() - n.y * plot.getHeight() * .25f};
}
juce::Point<float> CurveEditor::bendPoint(const CurveNode& a, const CurveNode& b) const
{
 return {visibleX((a.x+b.x)*.5f), plot.getCentreY() - CurveMath::segmentValue(a,b,(a.x+b.x)*.5f)*plot.getHeight()*.25f};
}
CurveNode CurveEditor::fromPoint(juce::Point<float> p, bool snap) const
{
 float x = CurveMath::clamp(viewStart+(p.x-plot.getX())/plot.getWidth()*viewWidth,0.f,1.f);
 float y = CurveMath::clamp((plot.getCentreY()-p.y)*4.f/plot.getHeight(),-2.f,2.f);
 if (snap) {
  const int length=juce::jlimit(0,4,juce::roundToInt(
      processor.apvts.getRawParameterValue("loopLength")->load()));
  x=ScratchTiming::snapNormalized(x,length);
  y=std::round(y*8.f)/8.f;
 }
 return {x,y,0.f};
}
void CurveEditor::invalidateGrid() { gridCache = {}; repaint(); }
void CurveEditor::paintGrid(juce::Graphics& g)
{
 const auto whole=juce::Rectangle<float>(0,0,designW,designH);
 g.fillAll(Theme::panel);
 g.setGradientFill(juce::ColourGradient(Theme::elevated,juce::Point<float>(0,0),
                                         Theme::panel,juce::Point<float>(designW,designH),false));
 g.fillRoundedRectangle(whole.reduced(.5f),6.f);
 g.setColour(Theme::edge.withAlpha(.75f));
 g.drawRoundedRectangle(whole.reduced(1.f),6.f,.8f);
 g.setColour(Theme::accent);
 g.setFont(Theme::font(10.f,true));
 g.drawText("SPEED / ENVELOPE",11,6,147,15,juce::Justification::centredLeft);
 g.setColour(Theme::muted);
 g.setFont(Theme::uiFont(8.f,true));
 // Edit shortcuts live in the tooltip; this space is reserved for playback status.
 g.setColour(Theme::plot);
 g.fillRoundedRectangle(plot,2.f);
 {
  juce::Graphics::ScopedSaveState state(g);
  g.reduceClipRegion(plot.toNearestInt());
  // The number of beat lines matches the selected loop length: 1/4 bar
  // is one beat, while a four-bar loop has 16 beats. At closer zooms,
  // 1/16-beat divisions become visible without cluttering the full view.
  const int length=juce::jlimit(0,4,juce::roundToInt(
      processor.apvts.getRawParameterValue("loopLength")->load()));
  const int beats=ScratchTiming::beatsForLengthChoice(length);
  const int steps=beats*(viewWidth<=.32f?16:4);
  const int stepsPerBeat=steps/beats;
  for(int i=0;i<=steps;++i) {
   const float x=visibleX(static_cast<float>(i)/steps);
   if(x<plot.getX()-1.f || x>plot.getRight()+1.f) continue;
   const bool beat=i%stepsPerBeat==0;
   const bool bar=i%(4*stepsPerBeat)==0;
   g.setColour(Theme::accent.withAlpha(bar?.21f:beat?.14f:.037f));
   g.drawVerticalLine(juce::roundToInt(x),plot.getY(),plot.getBottom());
  }
  for(int yStep=-2;yStep<=2;++yStep) {
   const float y=plot.getCentreY()-yStep*plot.getHeight()*.25f;
   g.setColour(yStep==0?Theme::accent.withAlpha(.28f):Theme::muted.withAlpha(.15f));
   g.drawHorizontalLine(juce::roundToInt(y),plot.getX(),plot.getRight());
  }
 }
 g.setFont(Theme::uiFont(9.f,true));
 const int rangeIndex=juce::jlimit(0,2,juce::roundToInt(processor.apvts.getRawParameterValue("speedRange")->load()));
 const float multiplier=rangeIndex==0?.5f:rangeIndex==1?1.f:2.f;
 for(int k=-2;k<=2;++k) {
  const float y=plot.getCentreY()-k*plot.getHeight()*.25f;
  g.setColour(k==0?Theme::accent:Theme::muted);
  const float speed=k*multiplier;
  const auto tick=(speed>0.f?"+":"")+juce::String(speed,rangeIndex==0?1:0)+"x";
  g.drawFittedText(tick,2,juce::roundToInt(y-7.f),28,14,juce::Justification::centredRight,1);
 }
 g.setColour(Theme::edge);
 g.drawRoundedRectangle(plot,2.f,1.f);
 // True scrollbar: the track depicts the full cycle; the handle is the visible range.
 const auto track=juce::Rectangle<float>(32.f,181.f,250.f,5.f);
 g.setColour(Theme::muted.withAlpha(.17f));
 g.fillRoundedRectangle(track,2.5f);
 const auto thumb=juce::Rectangle<float>(track.getX()+viewStart*track.getWidth(),track.getY(),track.getWidth()*viewWidth,track.getHeight());
 g.setColour(Theme::accent.withAlpha(.66f));
 g.fillRoundedRectangle(thumb,2.5f);
 g.setColour(Theme::muted);
 g.setFont(Theme::uiFont(8.2f,true));
 g.drawText("0",32,171,16,9,juce::Justification::centredLeft);
 g.setColour(Theme::muted.withAlpha(.78f));
 g.drawFittedText("SNAP 1/16 BEAT  |  ALT: FREE",69,171,173,10,juce::Justification::centred,1);
 g.setColour(Theme::muted);
 g.drawText("1 LOOP",244,171,38,9,juce::Justification::centredRight);
}
void CurveEditor::paint(juce::Graphics& g)
{
 g.fillAll(Theme::panel);
 juce::Graphics::ScopedSaveState root(g);
 g.addTransform(juce::AffineTransform::scale(getWidth()/designW,getHeight()/designH));
 if (!gridCache.isValid()) {
  // Rasterize static text/grid at a larger size than the base logical
  // layout so JUCE does not magnify a 291x193 bitmap on HiDPI screens.
  const int rasterScale=juce::jlimit(2,3,juce::roundToInt(
      std::max(getWidth()/designW,getHeight()/designH)*1.5f));
  gridCache=juce::Image(juce::Image::ARGB,
      juce::roundToInt(designW)*rasterScale,
      juce::roundToInt(designH)*rasterScale,true);
  juce::Graphics bg(gridCache);
  bg.addTransform(juce::AffineTransform::scale((float)rasterScale));
  paintGrid(bg);
 }
 g.drawImage(gridCache,juce::Rectangle<float>(0.f,0.f,designW,designH));
 const auto data=processor.curveSnapshot();
 const float capture=processor.captureProgress.load(std::memory_order_relaxed);
 if(capture>0.f && capture<1.f) {
  g.setColour(Theme::accent.withAlpha(.12f));
  g.fillRect(plot.getX()+1.f,plot.getBottom()-2.f,plot.getWidth()-2.f,1.5f);
  g.setColour(Theme::accent.withAlpha(.65f));
  g.fillRect(plot.getX()+1.f,plot.getBottom()-2.f,(plot.getWidth()-2.f)*capture,1.5f);
 }
 if(data && selected>=0 && selected<(int)data->nodes.size()){
  const auto& n=data->nodes[(size_t)selected];
  const int rangeIndex=juce::jlimit(0,2,juce::roundToInt(processor.apvts.getRawParameterValue("speedRange")->load()));
  const float fullRange=rangeIndex==0?1.f:rangeIndex==1?2.f:4.f;
  const auto detail="POINT "+juce::String(selected+1)+"    "+juce::String(n.x*100.f,1)+"%    "+
      juce::String(n.y*fullRange*.5f,2)+"x";
  g.setColour(Theme::elevated);
  g.fillRoundedRectangle(134.f,5.f,151.f,17.f,3.f);
  g.setColour(Theme::accent);
  g.setFont(Theme::uiFont(8.5f,true));
  g.drawFittedText(detail,137,7,145,12,juce::Justification::centredRight,1);
 } else {
  const float speed=processor.currentSpeed.load(std::memory_order_relaxed);
  const bool stopped=processor.apvts.getRawParameterValue("stopTrigger")->load()>.5f;
  juce::String status;
  if(stopped) status="STOPPED";
  else if(capture<.99f) status="CAPTURE "+juce::String(juce::roundToInt(capture*100.f))+"%";
  else if(std::abs(speed)<.03f) status="HOLD  0.00x";
  else status=(speed<0.f?"REV  ":"FWD  +")+juce::String(speed,2)+"x";
  g.setColour(Theme::accent.withAlpha(.10f));
  g.fillRoundedRectangle(166.f,6.f,119.f,15.f,3.f);
  g.setColour(stopped?Theme::muted:Theme::accent);
  g.setFont(Theme::uiFont(8.5f,true));
  g.drawFittedText(status,168,7,114,12,juce::Justification::centredRight,1);
 }
 if (!data || data->nodes.size()<2) return;
 const auto& nodes=data->nodes;
 juce::Graphics::ScopedSaveState clip(g);
 g.reduceClipRegion(plot.toNearestInt());
 const int guideIndex=selected>=0?selected:hovered;
 if(guideIndex>=0 && guideIndex<(int)nodes.size()) {
  const auto p=nodePoint(nodes[(size_t)guideIndex]);
  g.setColour(Theme::accent.withAlpha(.23f));
  g.drawLine(plot.getX(),p.y,plot.getRight(),p.y,.7f);
  g.drawLine(p.x,plot.getY(),p.x,plot.getBottom(),.7f);
 }
 juce::Path line;
 bool started=false;
 const float viewEnd=viewStart+viewWidth;
 // Only tessellate segments visible in the current zoom window. The
 // interpolation remains identical to the DSP, including curved midpoints.
 for(size_t i=1;i<nodes.size();++i) {
  const auto& a=nodes[i-1]; const auto& b=nodes[i];
  if(b.x<viewStart) continue;
  if(a.x>viewEnd) break;
  const float from=std::max(viewStart,a.x);
  const float to=std::min(viewEnd,b.x);
  if(to<from) continue;
  if(!started) {
   line.startNewSubPath(visibleX(from),plot.getCentreY()
       -CurveMath::segmentValue(a,b,from)*plot.getHeight()*.25f);
   started=true;
  }
  const int steps=juce::jlimit(2,56,juce::roundToInt(std::abs(visibleX(to)-visibleX(from))*.4f));
  for(int s=1;s<=steps;++s) {
   const float x=from+(to-from)*(float)s/steps;
   line.lineTo(visibleX(x),plot.getCentreY()-CurveMath::segmentValue(a,b,x)*plot.getHeight()*.25f);
  }
 }
 g.setColour(Theme::accent.withAlpha(.10f));
 g.strokePath(line,juce::PathStrokeType(5.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 g.setColour(Theme::accent.withAlpha(.92f));
 g.strokePath(line,juce::PathStrokeType(1.6f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 // Visible midpoint grips, so users can discover per-segment tension editing.
 for(size_t i=1;i<nodes.size();++i) {
  auto p=bendPoint(nodes[i-1],nodes[i]);
  if(p.x<plot.getX() || p.x>plot.getRight())continue;
  const bool hot=(int)i-1==hoveredBend || (int)i-1==bend;
  g.setColour(hot?Theme::white:Theme::accent.withAlpha(.50f));
  juce::Path diamond;
  const float r=hot?4.8f:3.5f;
  diamond.startNewSubPath(p.x,p.y-r);diamond.lineTo(p.x+r,p.y);diamond.lineTo(p.x,p.y+r);diamond.lineTo(p.x-r,p.y);diamond.closeSubPath();
  g.fillPath(diamond);
 }
 for(size_t i=0;i<nodes.size();++i) {
  const auto p=nodePoint(nodes[i]);
  if(p.x<plot.getX()-6.f||p.x>plot.getRight()+6.f)continue;
  const bool chosen=(int)i==selected;
  const bool hot=(int)i==hovered||chosen;
  const float r=chosen?5.f:hot?4.7f:3.7f;
  g.setColour(Theme::plot);
  g.fillEllipse(p.x-r-1.5f,p.y-r-1.5f,2*r+3,2*r+3);
  g.setColour(chosen?Theme::white:hot?Theme::accent.brighter(.20f):Theme::accent);
  g.fillEllipse(p.x-r,p.y-r,2*r,2*r);
  if(hot){g.setColour(Theme::accent.withAlpha(chosen?.82f:.40f));g.drawEllipse(p.x-r-2.7f,p.y-r-2.7f,2*r+5.4f,2*r+5.4f,1.f);}
 }
 const float play=processor.playhead.load(std::memory_order_relaxed);
 const float px=visibleX(play);
 g.setColour(Theme::accent.withAlpha(.07f));
 g.fillRect(px-2.f,plot.getY(),4.f,plot.getHeight());
 g.setColour(Theme::accent.withAlpha(.65f));
 g.drawVerticalLine(juce::roundToInt(px),plot.getY(),plot.getBottom());
 g.setColour(Theme::accent);
 juce::Path playheadFlag;
 playheadFlag.addTriangle(px-3.5f,plot.getY()+1.f,px+3.5f,plot.getY()+1.f,px,plot.getY()+7.f);
 g.fillPath(playheadFlag);
 for(int i=4;i>=0;--i) {
  float phase=play-i*.006f;
  if(phase<0)phase+=1.f;
  const float x=visibleX(phase),y=plot.getCentreY()-CurveMath::evaluate(nodes,phase)*plot.getHeight()*.25f;
  g.setColour(Theme::white.withAlpha((5.f-i)/5.f*.8f));
  g.fillEllipse(x-2.1f,y-2.1f,4.2f,4.2f);
 }
}
int CurveEditor::findNode(juce::Point<float> p,float radius) const
{
 if(!plot.expanded(radius).contains(p))return -1;
 auto data=processor.curveSnapshot();
 if(!data)return -1;
 int closest=-1;float best=radius;
 for(int i=0;i<(int)data->nodes.size();++i){float d=nodePoint(data->nodes[(size_t)i]).getDistanceFrom(p);if(d<best){best=d;closest=i;}}
 return closest;
}
int CurveEditor::findBend(juce::Point<float> p,float radius) const
{
 if(!plot.contains(p)) return -1;
 auto data=processor.curveSnapshot();
 if(!data)return -1;
 int closest=-1;float best=radius;
 for(int i=0;i+1<(int)data->nodes.size();++i){float d=bendPoint(data->nodes[(size_t)i],data->nodes[(size_t)i+1]).getDistanceFrom(p);if(d<best){best=d;closest=i;}}
 return closest;
}
void CurveEditor::timerCallback()
{
 const auto p=processor.playhead.load(std::memory_order_relaxed);
 const auto speed=processor.currentSpeed.load(std::memory_order_relaxed);
 const auto capture=processor.captureProgress.load(std::memory_order_relaxed);
 const auto version=processor.curveVersion.load(std::memory_order_relaxed);
 const bool stopped=processor.apvts.getRawParameterValue("stopTrigger")->load()>.5f;
 const int loop=juce::jlimit(0,4,juce::roundToInt(processor.apvts.getRawParameterValue("loopLength")->load()));
 const int range=juce::jlimit(0,2,juce::roundToInt(processor.apvts.getRawParameterValue("speedRange")->load()));
 if(range!=lastRangeIndex||loop!=lastLoopIndex){
  lastRangeIndex=range;lastLoopIndex=loop;invalidateGrid();return;
 }
 if(p!=lastPosition||std::abs(speed-lastSpeed)>.005f||
    std::abs(capture-lastCapture)>.005f||version!=lastCurveVersion||stopped!=lastStopped){
  lastPosition=p;lastSpeed=speed;lastCapture=capture;
  lastCurveVersion=version;lastStopped=stopped;
  repaint();
 }
}
void CurveEditor::saveUndo()
{
 undoStack.push_back({processor.curveSnapshot()->nodes,processor.getPresetName()});
 if(undoStack.size()>48) undoStack.erase(undoStack.begin());
 redoStack.clear();
}
void CurveEditor::undo()
{
 if(undoStack.empty()) return;
 redoStack.push_back({processor.curveSnapshot()->nodes,processor.getPresetName()});
 auto previous=std::move(undoStack.back());undoStack.pop_back();
 processor.setCurve(std::move(previous.nodes));
 processor.setPresetName(previous.presetName);
 selected=hovered=dragNode=bend=hoveredBend=-1; repaint();
}
void CurveEditor::redo()
{
 if(redoStack.empty()) return;
 undoStack.push_back({processor.curveSnapshot()->nodes,processor.getPresetName()});
 auto next=std::move(redoStack.back());redoStack.pop_back();
 processor.setCurve(std::move(next.nodes));
 processor.setPresetName(next.presetName);
 selected=hovered=dragNode=bend=hoveredBend=-1;repaint();
}
void CurveEditor::edit(const std::function<void(std::vector<CurveNode>&)>& f,bool customName)
{
 auto points=processor.curveSnapshot()->nodes;f(points);
 processor.setCurve(std::move(points));
 if(customName)processor.setPresetName("Custom");
 repaint();
}
void CurveEditor::setPreset(const std::vector<CurveNode>& points)
{
 saveUndo(); processor.setCurve(points); resetSelectionAndRepaint();
}
void CurveEditor::rememberExternalChange(){saveUndo();}
void CurveEditor::resetSelectionAndRepaint()
{
 selected=hovered=dragNode=bend=hoveredBend=-1;
 repaint();
}
void CurveEditor::mouseMove(const juce::MouseEvent& e)
{
 const auto p=logical(e.position);
 const auto newNode=findNode(p);
 const auto newBend=newNode<0?findBend(p):-1;
 if(newNode!=hovered||newBend!=hoveredBend){hovered=newNode;hoveredBend=newBend;repaint();}
 const auto type=(newNode>=0||newBend>=0)?juce::MouseCursor::PointingHandCursor:
     plot.contains(p)?juce::MouseCursor::CrosshairCursor:juce::MouseCursor::NormalCursor;
 setMouseCursor(juce::MouseCursor(type));
}
void CurveEditor::mouseExit(const juce::MouseEvent&){hovered=hoveredBend=-1;repaint();}
void CurveEditor::mouseDown(const juce::MouseEvent& e)
{
 grabKeyboardFocus();
 if(e.getNumberOfClicks()>1)return;
 const auto p=logical(e.position);
 if(e.mods.isPopupMenu()) {contextMenu(findNode(p));return;}
 if(p.y>=177.f && p.x>=28.f && p.x<=285.f){
  draggingScroll=true;scrollStart=viewStart;
  const float trackX=32.f+viewStart*250.f,trackW=250.f*viewWidth;
  if(p.x<trackX||p.x>trackX+trackW){viewStart=CurveMath::clamp((p.x-32.f)/250.f-viewWidth*.5f,0.f,1.f-viewWidth);scrollStart=viewStart;invalidateGrid();}
  return;
 }
 if(!plot.contains(p))return;
 dragNode=findNode(p);bend=dragNode<0?findBend(p):-1;didDrag=false;
 if(dragNode>=0){selected=dragNode;repaint();return;}
 if(bend>=0){selected=-1;repaint();return;}
 selected=-1;
 if(processor.curveSnapshot()->nodes.size()>=512)return;
 saveUndo();
 auto node=fromPoint(p,!e.mods.isAltDown());
 // Preserve existing points: adding near a snapped X moves/edits the existing point.
 edit([&](auto& points){points.push_back(node);});
 selected=findNode(nodePoint(node),11.f);
 dragNode=selected;
 didDrag=true; // insertion and the following drag are one undo gesture
}
void CurveEditor::mouseDrag(const juce::MouseEvent& e)
{
 const auto p=logical(e.position);
 if(draggingScroll){
  const float delta=(p.x-logical(e.mouseDownPosition).x)/250.f;
  viewStart=CurveMath::clamp(scrollStart+delta,0.f,1.f-viewWidth);
  invalidateGrid();return;
 }
 if(dragNode>=0){
  if(!didDrag){saveUndo();didDrag=true;}
  auto node=fromPoint(p,!e.mods.isAltDown());
  edit([&](auto& v){
   if(dragNode>=(int)v.size())return;
   const size_t i=(size_t)dragNode;
   node.tension=v[i].tension;
   if(i==0)node.x=0.f;
   else if(i+1==v.size())node.x=1.f;
   else{
    const float lo=v[i-1].x+.0002f,hi=v[i+1].x-.0002f;
    node.x=(lo<=hi)?CurveMath::clamp(node.x,lo,hi):v[i].x;
   }
   v[i]=node;
  });
 }else if(bend>=0){
  if(!didDrag){saveUndo();didDrag=true;}
  edit([&](auto& v){
   if(bend+1>=(int)v.size())return;
   auto& a=v[(size_t)bend];const auto& b=v[(size_t)bend+1];
   const float centre=(a.y+b.y)*.5f;
   const float target=(plot.getCentreY()-p.y)*4.f/plot.getHeight();
   a.tension=CurveMath::clamp(target-centre,-2.f,2.f);
  });didDrag=true;
 }
}
void CurveEditor::mouseUp(const juce::MouseEvent&)
{
 dragNode=bend=-1;draggingScroll=false;didDrag=false;repaint();
}
void CurveEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
 const int index=findNode(logical(e.position),11.f);
 if(index<0) return;
 if(processor.curveSnapshot()->nodes.size()<=2) return;
 if(index==0 || index+1==(int)processor.curveSnapshot()->nodes.size()) return;
 saveUndo();
 edit([index](auto& v){if(index>0&&index+1<(int)v.size())v.erase(v.begin()+index);});
 selected=dragNode=bend=-1;
}
void CurveEditor::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& w)
{
 if(w.deltaY==0.f)return;
 if(e.mods.isCtrlDown()||e.mods.isCommandDown()){
  const float cursor=logical(e.position).x;
  const float ratio=CurveMath::clamp((cursor-plot.getX())/plot.getWidth(),0.f,1.f);
  const float anchor=viewStart+ratio*viewWidth;
  viewWidth=CurveMath::clamp(viewWidth*(w.deltaY>0.f?.84f:1.19f),.125f,1.f);
  viewStart=CurveMath::clamp(anchor-ratio*viewWidth,0.f,1.f-viewWidth);
 }else{
  viewStart=CurveMath::clamp(viewStart-w.deltaY*.14f*viewWidth,0.f,1.f-viewWidth);
 }
 invalidateGrid();
}
bool CurveEditor::keyPressed(const juce::KeyPress& key)
{
 const auto m=key.getModifiers();
 if((m.isCtrlDown()||m.isCommandDown()) && (key.getKeyCode()=='z'||key.getKeyCode()=='Z')){if(m.isShiftDown())redo();else undo();return true;}
 if((m.isCtrlDown()||m.isCommandDown()) && (key.getKeyCode()=='y'||key.getKeyCode()=='Y')){redo();return true;}
 if(key.getKeyCode()==juce::KeyPress::deleteKey||key.getKeyCode()==juce::KeyPress::backspaceKey){
  const int at=selected>=0?selected:hovered;
  if(at>0&&at+1<(int)processor.curveSnapshot()->nodes.size()){
   saveUndo();edit([at](auto& v){v.erase(v.begin()+at);});selected=hovered=-1;
  }return true;
 }
 const int code=key.getKeyCode();
 if(selected>=0 && (code==juce::KeyPress::leftKey || code==juce::KeyPress::rightKey
   || code==juce::KeyPress::upKey || code==juce::KeyPress::downKey)){
  const int length=juce::jlimit(0,4,juce::roundToInt(
      processor.apvts.getRawParameterValue("loopLength")->load()));
  const int divisions=ScratchTiming::beatsForLengthChoice(length)*(m.isShiftDown()?64:16);
  const float dx=1.f/(float)divisions;
  const float dy=m.isShiftDown()?1.f/32.f:1.f/8.f;
  const int index=selected;
  saveUndo();
  edit([&](auto& v){
   if(index>=(int)v.size())return;
   auto& n=v[(size_t)index];
   if(code==juce::KeyPress::upKey)n.y=CurveMath::clamp(n.y+dy,-2.f,2.f);
   if(code==juce::KeyPress::downKey)n.y=CurveMath::clamp(n.y-dy,-2.f,2.f);
   if(index>0&&index+1<(int)v.size()){
    const float lo=v[(size_t)index-1].x+.0002f;
    const float hi=v[(size_t)index+1].x-.0002f;
    if(lo<=hi){
     if(code==juce::KeyPress::leftKey)n.x=CurveMath::clamp(n.x-dx,lo,hi);
     if(code==juce::KeyPress::rightKey)n.x=CurveMath::clamp(n.x+dx,lo,hi);
    }
   }
  });return true;
 }
 return false;
}
void CurveEditor::contextMenu(int hitNode)
{
 juce::PopupMenu menu;
 if(hitNode>0 && hitNode+1<(int)processor.curveSnapshot()->nodes.size()){
  menu.addItem(6,"Delete point");menu.addSeparator();
 }
 menu.addItem(1,"Reset curve");menu.addItem(2,"Invert speed");menu.addItem(3,"Reverse time");
 menu.addItem(4,"Stretch to 2x (clipped)");menu.addSeparator();
 menu.addItem(7,"Undo  (Ctrl+Z)",!undoStack.empty());
 menu.addItem(8,"Redo  (Ctrl+Y)",!redoStack.empty());
 juce::PopupMenu presets;
 for(int i=0;i<ScratchPresets::count();++i) presets.addItem(100+i,ScratchPresets::names[(size_t)i]);
 menu.addSubMenu("Factory presets",presets);
 juce::Component::SafePointer<CurveEditor> safe(this);
 menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),[safe,hitNode](int command){
  if(!safe||command==0)return;
  if(command==7){safe->undo();return;}
  if(command==8){safe->redo();return;}
  safe->saveUndo();
  if(command>=100 && command<100+ScratchPresets::count()){
   safe->processor.setCurve(ScratchPresets::make(command-100));
   safe->processor.setPresetName(ScratchPresets::names[(size_t)(command-100)]);
  } else safe->edit([command,hitNode](auto& nodes){
   switch(command){
    case 1:nodes={{0,0,0},{1,0,0}};break;
    case 2:for(auto& n:nodes){n.y=-n.y;n.tension=-n.tension;}break;
    case 3:{
     auto old=nodes;
     std::reverse(nodes.begin(),nodes.end());
     for(size_t i=0;i<nodes.size();++i){
      nodes[i].x=1.f-old[old.size()-1-i].x;
      if(i+1<nodes.size())nodes[i].tension=old[old.size()-2-i].tension;
      else nodes[i].tension=0.f;
     }
    }break;
    case 4:for(auto& n:nodes)n.x=CurveMath::clamp(n.x*2.f,0.f,1.f);break;
    case 6:if(hitNode>0 && hitNode+1<(int)nodes.size())nodes.erase(nodes.begin()+hitNode);break;
   }
  });
  safe->resetSelectionAndRepaint();
 });
}
