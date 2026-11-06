#include "FooterBar.h"
#include "PresetBank.h"

FooterBar::FooterBar(ScratchPadAudioProcessor& p) : processor(p)
{
 setMouseCursor(juce::MouseCursor::NormalCursor);
 nameEditor.setMultiLine(false);
 nameEditor.setReturnKeyStartsNewLine(false);
 nameEditor.setColour(juce::TextEditor::backgroundColourId,Theme::plot);
 nameEditor.setColour(juce::TextEditor::textColourId,Theme::light);
 nameEditor.setColour(juce::TextEditor::highlightColourId,Theme::accent2);
 nameEditor.setColour(juce::TextEditor::outlineColourId,Theme::accent);
 nameEditor.setFont(Theme::uiFont(11.f));
 nameEditor.setVisible(false);
 addChildComponent(nameEditor);
 nameEditor.onReturnKey=[this]{finishRename(true);};
 nameEditor.onEscapeKey=[this]{finishRename(false);};
 nameEditor.onFocusLost=[this]{finishRename(true);};
 syncPresetIndex();
}
juce::Point<float> FooterBar::logical(juce::Point<float> p) const
{
 return {p.x*designW/std::max(1,getWidth()),p.y*designH/std::max(1,getHeight())};
}
void FooterBar::resized()
{
 nameEditor.setBounds(juce::roundToInt(286.f*getWidth()/designW),juce::roundToInt(21.f*getHeight()/designH),
                      juce::roundToInt(108.f*getWidth()/designW),juce::roundToInt(17.f*getHeight()/designH));
 nameEditor.setFont(Theme::uiFont(11.f*getHeight()/designH));
}
int FooterBar::hitTest(juce::Point<float> p) const
{
 if(p.x>=197.f&&p.x<257.f) return p.y<20.f?1:2;
 if(p.y<17.f)return -1;
 if(p.x>=258.f&&p.x<282.f) return 3;
 if(p.x>=398.f&&p.x<=423.f) return 4;
 if(p.x>=283.f&&p.x<398.f) return 5;
 return -1;
}
void FooterBar::mouseMove(const juce::MouseEvent& e)
{
 const int action=hitTest(logical(e.position));
 if(action!=hoveredAction){hoveredAction=action;
  setMouseCursor(action>=0?juce::MouseCursor::PointingHandCursor:juce::MouseCursor::NormalCursor);
  repaint();}
}
void FooterBar::mouseExit(const juce::MouseEvent&)
{
 hoveredAction=-1;setMouseCursor(juce::MouseCursor::NormalCursor);repaint();
}
void FooterBar::paint(juce::Graphics& g)
{
 juce::Graphics::ScopedSaveState saved(g);
 g.addTransform(juce::AffineTransform::scale(getWidth()/designW,getHeight()/designH));
 // The logotype has a single intentional diagonal cut; everything else
 // uses quiet dark surfaces so preset names and controls are legible.
 juce::Path brand;
 brand.startNewSubPath(0,3);brand.lineTo(181,3);brand.lineTo(193,21);
 brand.lineTo(181,39);brand.lineTo(0,39);brand.closeSubPath();
 g.setColour(Theme::plot);g.fillPath(brand);
 g.setColour(Theme::edge);g.strokePath(brand,juce::PathStrokeType(.8f));
 g.setColour(Theme::accent);g.setFont(Theme::font(19.f,true));
 g.drawText("SKELLY",13,5,97,24,juce::Justification::centredLeft);
 g.setColour(Theme::muted);g.setFont(Theme::uiFont(9.1f,true));
 g.drawText("SCRATCHPAD",107,13,77,12,juce::Justification::centredLeft);
 g.setColour(Theme::accent2.withAlpha(.7f));g.drawLine(13,34,99,34,1.f);
 // Two small but clearly separated text buttons.
 for(int action=1;action<=2;++action){
  const float y=action==1?3.f:22.f;
  if(hoveredAction==action){g.setColour(Theme::elevated);g.fillRoundedRectangle(197.f,y,59.f,16.f,3.f);}
  g.setColour(hoveredAction==action?Theme::accent:Theme::muted);
  g.setFont(Theme::uiFont(10.f,true));
  g.drawText(action==1?"MENU":"RENAME",201,(int)y,52,16,juce::Justification::centredLeft);
 }
 // Compact preset control with crisp arrow hit targets and a proper label.
 g.setColour(Theme::muted);g.setFont(Theme::uiFont(8.5f,true));
 g.drawText("FACTORY / USER PRESET",283,3,115,13,juce::Justification::centred);
 for(int arrow=0;arrow<2;++arrow){
  const float x=arrow==0?259.f:400.f;
  const bool hot=hoveredAction==(arrow==0?3:4);
  g.setColour(hot?Theme::edge:Theme::elevated);
  g.fillRoundedRectangle(x,19.f,19.f,20.f,4.f);
  g.setColour(Theme::accent.withAlpha(hot?1.f:.82f));
  juce::Path chevron;
  const float cx=x+9.5f,cy=29.f;
  if(arrow==0){chevron.startNewSubPath(cx+2.f,cy-4.f);chevron.lineTo(cx-2.f,cy);chevron.lineTo(cx+2.f,cy+4.f);}
  else{chevron.startNewSubPath(cx-2.f,cy-4.f);chevron.lineTo(cx+2.f,cy);chevron.lineTo(cx-2.f,cy+4.f);}
  g.strokePath(chevron,juce::PathStrokeType(1.8f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 }
 if(!renaming){
  if(hoveredAction==5){g.setColour(Theme::elevated);g.fillRoundedRectangle(282.f,19.f,116.f,20.f,4.f);}
  g.setColour(Theme::light);g.setFont(Theme::uiFont(11.f,true));
  g.drawFittedText(processor.getPresetName(),284,20,112,18,juce::Justification::centred,1);
 }
}
void FooterBar::mouseUp(const juce::MouseEvent& e)
{
 switch(hitTest(logical(e.position))){
  case 1:menu();break;
  case 2:rename();break;
  case 3:applyPreset((preset+ScratchPresets::count()-1)%ScratchPresets::count());break;
  case 4:applyPreset((preset+1)%ScratchPresets::count());break;
  case 5:rename();break;
  default:break;
 }
}
void FooterBar::syncPresetIndex()
{
 for(int i=0;i<ScratchPresets::count();++i)
  if(processor.getPresetName()==ScratchPresets::names[(size_t)i]){
   preset=i;
   return;
  }
}
void FooterBar::applyPreset(int index)
{
 if(beforePresetChange)beforePresetChange();
 preset=juce::jlimit(0,ScratchPresets::count()-1,index);
 processor.setCurve(ScratchPresets::make(preset));
 processor.setPresetName(ScratchPresets::names[(size_t)preset]);
 if(loadPreset)loadPreset(preset);
 repaint();
}
void FooterBar::rename()
{
 if(renaming)return;
 renaming=true;
 nameEditor.setText(processor.getPresetName(),juce::dontSendNotification);
 nameEditor.setVisible(true);
 nameEditor.grabKeyboardFocus();
 nameEditor.selectAll();
 repaint();
}
void FooterBar::finishRename(bool commit)
{
 if(!renaming)return;
 if(commit){
  auto name=nameEditor.getText().trim().substring(0,48);
  if(name.isNotEmpty())processor.setPresetName(name);
 }
 renaming=false;
 nameEditor.setVisible(false);
 repaint();
}
void FooterBar::menu()
{
 juce::PopupMenu m;
 m.addItem(1,"Save preset...");m.addItem(2,"Load preset...");
 m.addItem(3,"Initialize curve");m.addItem(4,"About ScratchPad");
 juce::PopupMenu bank;
 for(int i=0;i<ScratchPresets::count();++i) bank.addItem(100+i,ScratchPresets::names[(size_t)i]);
 m.addSubMenu("Factory presets ("+juce::String(ScratchPresets::count())+")",bank);
 juce::PopupMenu lengths;
 const auto currentLength=juce::roundToInt(processor.apvts.getRawParameterValue("loopLength")->load());
 const juce::StringArray lengthNames{"1/4 bar","1/2 bar","1 bar","2 bars","4 bars"};
 for(int i=0;i<5;++i)lengths.addItem(40+i,lengthNames[i],true,i==currentLength);
 m.addSubMenu("Loop length",lengths);
 juce::PopupMenu ranges;
 const auto currentRange=juce::roundToInt(processor.apvts.getRawParameterValue("speedRange")->load());
 const juce::StringArray rangeNames{"±1x","±2x","±4x"};
 for(int i=0;i<3;++i)ranges.addItem(50+i,rangeNames[i],true,i==currentRange);
 m.addSubMenu("Speed range",ranges);
 m.addItem(60,"Tempo sync",true,processor.apvts.getRawParameterValue("sync")->load()>.5f);
 m.addSeparator();
 juce::Component::SafePointer<FooterBar> safe(this);
 m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),[safe](int command){
  if(!safe||command==0)return;
  if(command>=100&&command<100+ScratchPresets::count()){
   safe->applyPreset(command-100);
   return;
  }
  if((command>=40&&command<45)||(command>=50&&command<53)||command==60){
   const bool isLength=command>=40&&command<45;
   const bool isRange=command>=50&&command<53;
   const char* id=isLength?"loopLength":isRange?"speedRange":"sync";
   auto* parameter=safe->processor.apvts.getParameter(id);
   if(parameter){
    const float requested=isLength?(float)(command-40):isRange?(float)(command-50):
       (safe->processor.apvts.getRawParameterValue("sync")->load()>.5f?0.f:1.f);
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost(parameter->convertTo0to1(requested));
    parameter->endChangeGesture();
   }
   return;
  }
  if(command==3){
   if(safe->beforePresetChange)safe->beforePresetChange();
   safe->processor.setCurve({{0.f,1.f,0.f},{1.f,0.f,0.f}});
   safe->processor.setPresetName("Init");
   if(safe->loadPreset)safe->loadPreset(-1);
   safe->repaint();return;
  }
  if(command==4){
   juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"ScratchPad",
    "Skelly Neon v1.9. Add or move dots, drag diamond handles to bend, Alt for free editing, Ctrl+wheel to zoom, Ctrl+Z to undo. Scratch Amount 0% is dry; STOP is automatable.");
   return;
  }
  if(command==1||command==2){
   auto chooser=std::make_shared<juce::FileChooser>(command==1?"Save ScratchPad preset":"Load ScratchPad preset",
      juce::File{},"*.scratchpad");
   const int flags=command==1?
      juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles:
      juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles;
   chooser->launchAsync(flags,[safe,chooser,command](const juce::FileChooser& fc){
    if(!safe)return;
    auto file=fc.getResult();if(file==juce::File{})return;
    if(command==1){
     juce::MemoryBlock data;safe->processor.getStateInformation(data);
     if(!file.withFileExtension(".scratchpad").replaceWithData(data.getData(),data.getSize()))
      juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
          "ScratchPad", "Preset could not be saved. Check folder permissions.");
    }else{
     juce::MemoryBlock data;
     if(file.loadFileAsData(data)){
      if(safe->beforePresetChange)safe->beforePresetChange();
      safe->processor.setStateInformation(data.getData(),(int)data.getSize());
      safe->syncPresetIndex();
      if(safe->loadPreset)safe->loadPreset(-1);
     } else {
      juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
          "ScratchPad", "Preset could not be opened. Check the file and its permissions.");
     }
    }
    safe->repaint();
   });
  }
 });
}
