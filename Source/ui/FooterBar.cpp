#include "FooterBar.h"
#include "PresetBank.h"

FooterBar::FooterBar(ScratchPadAudioProcessor& p) : processor(p)
{
 setMouseCursor(juce::MouseCursor::NormalCursor);
 nameEditor.setMultiLine(false);
 nameEditor.setReturnKeyStartsNewLine(false);
 nameEditor.setColour(juce::TextEditor::backgroundColourId,Theme::plot);
 nameEditor.setColour(juce::TextEditor::textColourId,Theme::light);
 nameEditor.setColour(juce::TextEditor::highlightColourId,Theme::accent.withAlpha(.30f));
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
 Theme::drawCard(g,juce::Rectangle<float>(.5f,.5f,423.f,41.f));
 // Official Skell skull: compact vector bone mark, no separate font/image.
 Theme::drawSkull(g,10.f,6.f,.96f);
 // Measure the *actual glyphs* rather than using a hard-coded PAD x-offset.
 // The title now reads as one unit at every supported window size.
 const auto nameFont=Theme::font(17.f,true);
 juce::GlyphArrangement glyphs;
 glyphs.addLineOfText(nameFont,"SKELL",0.f,0.f);
 const float skelWidth=glyphs.getBoundingBox(0,-1,true).getWidth();
 const int padX=juce::roundToInt(42.f+skelWidth+2.f);
 g.setFont(nameFont);
 g.setColour(Theme::light);
 g.drawText("SKELL",42,5,juce::roundToInt(skelWidth+2.f),22,juce::Justification::centredLeft);
 g.setColour(Theme::accent);
 g.drawText("PAD",padX,5,56,22,juce::Justification::centredLeft);
 g.setColour(Theme::muted);
 g.setFont(Theme::uiFont(8.f));
 g.drawText("SCRATCH / ENVELOPE",43,26,145,11,juce::Justification::centredLeft);
 g.setColour(Theme::edge);
 g.drawLine(191.f,7.f,191.f,35.f,1.f);
 // Compact text actions, preserving the existing hit areas and gestures.
 for(int action=1;action<=2;++action){
  const float y=action==1?3.f:22.f;
  const bool over=hoveredAction==action;
  if(over){g.setColour(Theme::elevated);g.fillRect(juce::Rectangle<float>(196.f,y,59.f,16.f));}
  g.setColour(over?Theme::accent:Theme::light);
  g.setFont(Theme::uiFont(10.f,true));
  g.drawText(action==1?"MENU":"RENAME",200,(int)y,54,16,juce::Justification::centredLeft);
 }
 g.setColour(Theme::muted);
 g.setFont(Theme::uiFont(8.5f,true));
 g.drawText("PRESET",282,4,117,12,juce::Justification::centred);
 for(int arrow=0;arrow<2;++arrow){
  const float x=arrow==0?259.f:400.f;
  const bool hot=hoveredAction==(arrow==0?3:4);
  g.setColour(hot?Theme::elevated:Theme::panel);
  g.fillRect(juce::Rectangle<float>(x,19.f,19.f,20.f));
  g.setColour(hot?Theme::accent:Theme::edge);
  g.drawRect(juce::Rectangle<float>(x,19.f,19.f,20.f),1.f);
  g.setColour(hot?Theme::accent:Theme::light);
  juce::Path chevron;
  const float cx=x+9.5f,cy=29.f;
  if(arrow==0){chevron.startNewSubPath(cx+2.f,cy-4.f);chevron.lineTo(cx-2.f,cy);chevron.lineTo(cx+2.f,cy+4.f);}
  else{chevron.startNewSubPath(cx-2.f,cy-4.f);chevron.lineTo(cx+2.f,cy);chevron.lineTo(cx-2.f,cy+4.f);}
  g.strokePath(chevron,juce::PathStrokeType(1.6f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 }
 if(!renaming){
  if(hoveredAction==5){g.setColour(Theme::elevated);g.fillRect(juce::Rectangle<float>(282.f,19.f,116.f,20.f));}
  g.setColour(Theme::light);
  g.setFont(Theme::uiFont(10.8f,true));
  g.drawFittedText(processor.getPresetName(),284,20,112,18,juce::Justification::centred,1);
 }
}
void FooterBar::mouseUp(const juce::MouseEvent& e)
{
 const int clicked=hitTest(logical(e.position));
 if(clicked==3 || clicked==4)syncPresetIndex();
 switch(clicked){
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
 const auto name=processor.getPresetName();
 for(int i=0;i<ScratchPresets::count();++i)
  if(name==ScratchPresets::names[(size_t)i]){
   preset=i;
   return;
  }
 // A renamed or custom curve retains the last selected factory position.
}
void FooterBar::refreshPresetState()
{
 syncPresetIndex();
 repaint();
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
 m.addItem(3,"Initialize curve");m.addItem(5,"Guided tutorial...");m.addItem(4,"About SKELL PAD");
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
 m.addItem(61,"Loop Lock (capture next loop)",true,processor.apvts.getRawParameterValue("loopLock")->load()>.5f);
 m.addSeparator();
 juce::Component::SafePointer<FooterBar> safe(this);
 m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),[safe](int command){
  if(!safe||command==0)return;
  if(command>=100&&command<100+ScratchPresets::count()){
   safe->applyPreset(command-100);
   return;
  }
  if((command>=40&&command<45)||(command>=50&&command<53)||command==60||command==61){
   const bool isLength=command>=40&&command<45;
   const bool isRange=command>=50&&command<53;
   const char* id=isLength?"loopLength":isRange?"speedRange":command==60?"sync":"loopLock";
   auto* parameter=safe->processor.apvts.getParameter(id);
   if(parameter){
    const float requested=isLength?(float)(command-40):isRange?(float)(command-50):
       (safe->processor.apvts.getRawParameterValue(id)->load()>.5f?0.f:1.f);
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost(parameter->convertTo0to1(requested));
    parameter->endChangeGesture();
   }
   return;
  }
  if(command==5){if(safe->requestTutorial)safe->requestTutorial();return;}
  if(command==3){
   if(safe->beforePresetChange)safe->beforePresetChange();
   safe->processor.setCurve({{0.f,1.f,0.f},{1.f,0.f,0.f}});
   safe->processor.setPresetName("Init");
   if(safe->loadPreset)safe->loadPreset(-1);
   safe->repaint();return;
  }
  if(command==4){
   juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"ScratchPad",
    "Skell Pad Bone v1.19.0. Click to add points; drag circles to move, diamonds to bend. Alt: free editing; Ctrl+wheel: zoom; F: fit; Ctrl+Z: undo. Scratch 0% is dry. STOP mutes output; LOOP LOCK records one full incoming loop without overwriting it.");
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
