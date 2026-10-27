#include "CurveEditor.h"
#include "PresetBank.h"
#include <algorithm>
#include <cmath>

CurveEditor::CurveEditor(ScratchPadAudioProcessor& p) : processor(p)
{
 setOpaque(true);
 setMouseCursor(juce::MouseCursor::PointingHandCursor);
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
 if (snap) { x=std::round(x*64.f)/64.f; y=std::round(y*8.f)/8.f; }
 return {x,y,0.f};
}
void CurveEditor::invalidateGrid() { gridCache = {}; repaint(); }
void CurveEditor::paintGrid(juce::Graphics& g)
{
 const auto whole=juce::Rectangle<float>(0,0,designW,designH);
 g.fillAll(Theme::panel);
 g.setColour(Theme::elevated);
 g.fillRoundedRectangle(whole.reduced(.5f), 6.f);
 g.setColour(Theme::edge);
 g.drawRoundedRectangle(whole.reduced(1.f),6.f,1.f);
 g.setColour(Theme::accent);
 g.setFont(Theme::font(10.f,true));
 g.drawText("SPEED / ENVELOPE",11,6,146,14,juce::Justification::centredLeft);
 g.setColour(Theme::muted);
 g.setFont(Theme::font(8.2f));
 g.drawText("ALT: FREE  /  CTRL: ZOOM",146,6,136,14,juce::Justification::centredRight);
 g.setColour(Theme::plot);
 g.fillRoundedRectangle(plot,2.f);
 {
  juce::Graphics::ScopedSaveState state(g);
  g.reduceClipRegion(plot.toNearestInt());
  // A loop spans 4 beat divisions. Subdivisions remain musical when zoomed.
  for(int i=0;i<=64;++i) {
   const float x=visibleX(i/64.f);
   if (x<plot.getX()-1.f || x>plot.getRight()+1.f) continue;
   const bool beat=i%16==0;
   g.setColour(Theme::accent.withAlpha(beat?.24f:.095f));
   g.drawVerticalLine(juce::roundToInt(x),plot.getY(),plot.getBottom());
  }
  for(int yStep=-2;yStep<=2;++yStep) {
   const float y=plot.getCentreY()-yStep*plot.getHeight()*.25f;
   g.setColour(yStep==0?Theme::accent.withAlpha(.38f):Theme::muted.withAlpha(.23f));
   g.drawHorizontalLine(juce::roundToInt(y),plot.getX(),plot.getRight());
  }
 }
 g.setFont(Theme::font(9.f));
 for(int k=-2;k<=2;++k) {
  const float y=plot.getCentreY()-k*plot.getHeight()*.25f;
  g.setColour(k==0?Theme::accent:Theme::muted);
  g.drawFittedText((k>0?"+":"")+juce::String(k)+"x",3,juce::roundToInt(y-7.f),26,14,juce::Justification::centredRight,1);
 }
 g.setColour(Theme::edge);
 g.drawRoundedRectangle(plot,2.f,1.f);
 // True scrollbar: the track depicts the full cycle; the handle is the visible range.
 const auto track=juce::Rectangle<float>(32.f,181.f,250.f,5.f);
 g.setColour(Theme::muted.withAlpha(.22f));
 g.fillRoundedRectangle(track,2.5f);
 const auto thumb=juce::Rectangle<float>(track.getX()+viewStart*track.getWidth(),track.getY(),track.getWidth()*viewWidth,track.getHeight());
 g.setColour(Theme::accent.withAlpha(.75f));
 g.fillRoundedRectangle(thumb,2.5f);
 g.setColour(Theme::muted);
 g.setFont(Theme::font(7.7f));
 g.drawText("0",32,171,16,9,juce::Justification::centredLeft);
 g.drawText("1 LOOP",244,171,38,9,juce::Justification::centredRight);
}
void CurveEditor::paint(juce::Graphics& g)
{
 g.fillAll(Theme::panel);
 juce::Graphics::ScopedSaveState root(g);
 g.addTransform(juce::AffineTransform::scale(getWidth()/designW,getHeight()/designH));
 if (!gridCache.isValid()) {
  gridCache = juce::Image(juce::Image::ARGB,(int)designW,(int)designH,true);
  juce::Graphics bg(gridCache);
  paintGrid(bg);
 }
 g.drawImageAt(gridCache,0,0);
 const auto data=processor.curveSnapshot();
 if (!data || data->nodes.size()<2) return;
 const auto& nodes=data->nodes;
 juce::Graphics::ScopedSaveState clip(g);
 g.reduceClipRegion(plot.toNearestInt());
 juce::Path line;
 bool started=false;
 // Same quadratic tension interpolation as the audio DSP; adaptive line steps.
 for(size_t i=1;i<nodes.size();++i) {
  const auto& a=nodes[i-1]; const auto& b=nodes[i];
  if(!started) {line.startNewSubPath(nodePoint(a));started=true;}
  const int steps=juce::jlimit(2,56,juce::roundToInt(std::abs(visibleX(b.x)-visibleX(a.x))*.4f));
  for(int s=1;s<=steps;++s) {
   const float x=a.x+(b.x-a.x)*(float)s/steps;
   line.lineTo(visibleX(x),plot.getCentreY()-CurveMath::segmentValue(a,b,x)*plot.getHeight()*.25f);
  }
 }
 g.setColour(Theme::accent.withAlpha(.14f));
 g.strokePath(line,juce::PathStrokeType(6.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 g.setColour(Theme::accent);
 g.strokePath(line,juce::PathStrokeType(1.8f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 // Visible midpoint grips, so users can discover per-segment tension editing.
 for(size_t i=1;i<nodes.size();++i) {
  auto p=bendPoint(nodes[i-1],nodes[i]);
  if(p.x<plot.getX() || p.x>plot.getRight())continue;
  const bool hot=(int)i-1==hoveredBend || (int)i-1==bend;
  g.setColour(hot?Theme::accent:Theme::muted.withAlpha(.55f));
  juce::Path diamond;
  const float r=hot?4.1f:2.8f;
  diamond.startNewSubPath(p.x,p.y-r);diamond.lineTo(p.x+r,p.y);diamond.lineTo(p.x,p.y+r);diamond.lineTo(p.x-r,p.y);diamond.closeSubPath();
  g.fillPath(diamond);
 }
 for(size_t i=0;i<nodes.size();++i) {
  const auto p=nodePoint(nodes[i]);
  const bool hot=(int)i==hovered||(int)i==selected;
  const float r=hot?5.2f:3.6f;
  g.setColour(Theme::plot);
  g.fillEllipse(p.x-r-1.5f,p.y-r-1.5f,2*r+3,2*r+3);
  g.setColour(hot?Theme::white:Theme::accent);
  g.fillEllipse(p.x-r,p.y-r,2*r,2*r);
  if(hot){g.setColour(Theme::accent.withAlpha(.48f));g.drawEllipse(p.x-r-2.8f,p.y-r-2.8f,2*r+5.6f,2*r+5.6f,1.f);}
 }
 const float play=processor.playhead.load(std::memory_order_relaxed);
 const float px=visibleX(play);
 g.setColour(Theme::accent.withAlpha(.55f));
 g.drawVerticalLine(juce::roundToInt(px),plot.getY(),plot.getBottom());
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
void CurveEditor::timerCallback(){const auto p=processor.playhead.load(std::memory_order_relaxed);if(p!=lastPosition){lastPosition=p;repaint();}}
void CurveEditor::saveUndo()
{
 undoStack.push_back(processor.curveSnapshot()->nodes);
 if(undoStack.size()>48) undoStack.erase(undoStack.begin());
 redoStack.clear();
}
void CurveEditor::undo()
{
 if(undoStack.empty()) return;
 redoStack.push_back(processor.curveSnapshot()->nodes);
 processor.setCurve(std::move(undoStack.back())); undoStack.pop_back();
 processor.presetName="Custom"; selected=hovered=bend=hoveredBend=-1; repaint();
}
void CurveEditor::redo()
{
 if(redoStack.empty()) return;
 undoStack.push_back(processor.curveSnapshot()->nodes);
 processor.setCurve(std::move(redoStack.back()));redoStack.pop_back();
 processor.presetName="Custom";selected=hovered=bend=hoveredBend=-1;repaint();
}
void CurveEditor::edit(const std::function<void(std::vector<CurveNode>&)>& f,bool customName)
{
 auto points=processor.curveSnapshot()->nodes;f(points);
 processor.setCurve(std::move(points));
 if(customName)processor.presetName="Custom";
 repaint();
}
void CurveEditor::setPreset(const std::vector<CurveNode>& points)
{
 saveUndo(); processor.setCurve(points); selected=hovered=bend=hoveredBend=-1; repaint();
}
void CurveEditor::mouseMove(const juce::MouseEvent& e)
{
 const auto p=logical(e.position);
 const auto newNode=findNode(p);
 const auto newBend=newNode<0?findBend(p):-1;
 if(newNode!=hovered||newBend!=hoveredBend){hovered=newNode;hoveredBend=newBend;repaint();}
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
 selected=findNode(p);bend=selected<0?findBend(p):-1;didDrag=false;
 if(selected>=0||bend>=0){saveUndo();return;}
 if(processor.curveSnapshot()->nodes.size()>=512)return;
 saveUndo();
 auto node=fromPoint(p,!e.mods.isAltDown());
 // Preserve existing points: adding near a snapped X moves/edits the existing point.
 edit([&](auto& points){points.push_back(node);});
 selected=findNode(nodePoint(node),11.f);
}
void CurveEditor::mouseDrag(const juce::MouseEvent& e)
{
 const auto p=logical(e.position);
 if(draggingScroll){
  const float delta=(p.x-logical(e.mouseDownPosition).x)/250.f;
  viewStart=CurveMath::clamp(scrollStart+delta,0.f,1.f-viewWidth);
  invalidateGrid();return;
 }
 if(selected>=0){
  auto node=fromPoint(p,!e.mods.isAltDown());
  edit([&](auto& v){
   if(selected>=(int)v.size())return;
   const size_t i=(size_t)selected;
   node.tension=v[i].tension;
   if(i==0)node.x=0.f;
   else if(i+1==v.size())node.x=1.f;
   else{
    const float lo=v[i-1].x+.0002f,hi=v[i+1].x-.0002f;
    node.x=(lo<=hi)?CurveMath::clamp(node.x,lo,hi):v[i].x;
   }
   v[i]=node;
  });didDrag=true;
 }else if(bend>=0){
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
 selected=bend=-1;draggingScroll=false;didDrag=false;repaint();
}
void CurveEditor::mouseDoubleClick(const juce::MouseEvent& e)
{
 const int index=findNode(logical(e.position),11.f);
 if(index<0) return;
 if(processor.curveSnapshot()->nodes.size()<=2) return;
 if(index==0 || index+1==(int)processor.curveSnapshot()->nodes.size()) return;
 saveUndo();
 edit([index](auto& v){if(index>0&&index+1<(int)v.size())v.erase(v.begin()+index);});
 selected=bend=-1;
}
void CurveEditor::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& w)
{
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
  if(hovered>0&&hovered+1<(int)processor.curveSnapshot()->nodes.size()){
   const int at=hovered;saveUndo();edit([at](auto& v){v.erase(v.begin()+at);});hovered=-1;
  }return true;
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
   safe->processor.presetName=ScratchPresets::names[(size_t)(command-100)];
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
  safe->selected=safe->hovered=safe->bend=safe->hoveredBend=-1;
  safe->repaint();
 });
}
