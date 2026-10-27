#include "FooterBar.h"
#include "PresetBank.h"

FooterBar::FooterBar(ScratchPadAudioProcessor& p) : processor(p)
{
 setMouseCursor(juce::MouseCursor::PointingHandCursor);
 nameEditor.setMultiLine(false);
 nameEditor.setReturnKeyStartsNewLine(false);
 nameEditor.setColour(juce::TextEditor::backgroundColourId,Theme::plot);
 nameEditor.setColour(juce::TextEditor::textColourId,Theme::light);
 nameEditor.setColour(juce::TextEditor::highlightColourId,Theme::accent2);
 nameEditor.setColour(juce::TextEditor::outlineColourId,Theme::accent);
 nameEditor.setFont(Theme::font(11.f));
 nameEditor.setVisible(false);
 addChildComponent(nameEditor);
 nameEditor.onReturnKey=[this]{finishRename(true);};
 nameEditor.onEscapeKey=[this]{finishRename(false);};
 nameEditor.onFocusLost=[this]{finishRename(true);};
}
juce::Point<float> FooterBar::logical(juce::Point<float> p) const
{
 return {p.x*designW/std::max(1,getWidth()),p.y*designH/std::max(1,getHeight())};
}
void FooterBar::resized()
{
 nameEditor.setBounds(juce::roundToInt(286.f*getWidth()/designW),juce::roundToInt(21.f*getHeight()/designH),
                      juce::roundToInt(108.f*getWidth()/designW),juce::roundToInt(17.f*getHeight()/designH));
 nameEditor.setFont(Theme::font(11.f*getHeight()/designH));
}
void FooterBar::paint(juce::Graphics& g)
{
 juce::Graphics::ScopedSaveState saved(g);
 g.addTransform(juce::AffineTransform::scale(getWidth()/designW,getHeight()/designH));
 juce::Path panel;
 panel.startNewSubPath(0,3);panel.lineTo(184,3);panel.lineTo(194,21);panel.lineTo(184,39);
 panel.lineTo(0,39);panel.closeSubPath();
 g.setColour(Theme::plot);g.fillPath(panel);
 g.setColour(Theme::accent.withAlpha(.60f));g.strokePath(panel,juce::PathStrokeType(.85f));
 g.setColour(Theme::accent);g.setFont(Theme::font(20.f,true));
 g.drawText("SKELLY",12,6,96,22,juce::Justification::centredLeft);
 g.setColour(Theme::muted);g.setFont(Theme::font(11.f,true));
 g.drawText("/ SCRATCHPAD",100,11,84,14,juce::Justification::centredLeft);
 g.setColour(Theme::accent2);g.drawLine(12,33,116,33,1.f);
 g.setColour(Theme::muted);g.setFont(Theme::font(7.f));
 g.drawText("ENVELOPE ENGINE",119,29,69,9,juce::Justification::centredRight);
 g.setColour(Theme::accent);g.setFont(Theme::font(11.f,true));
 g.drawText("MENU",200,4,56,14,juce::Justification::centredLeft);
 g.drawText("RENAME",200,22,58,14,juce::Justification::centredLeft);
 // Arrow chips are separate target zones for reliable preset navigation.
 g.setColour(Theme::elevated);
 g.fillRoundedRectangle(259.f,20.f,19.f,18.f,4.f);
 g.fillRoundedRectangle(400.f,20.f,19.f,18.f,4.f);
 g.setColour(Theme::accent);g.setFont(Theme::font(16.f,true));
 g.drawText(juce::String::fromUTF8("\xE2\x97\x82"),259,19,19,19,juce::Justification::centred);
 g.drawText(juce::String::fromUTF8("\xE2\x96\xB8"),400,19,19,19,juce::Justification::centred);
 g.setColour(Theme::muted);g.setFont(Theme::font(8.f));
 g.drawText("PRESET",288,5,107,11,juce::Justification::centred);
 if(!renaming){
  g.setColour(Theme::light);g.setFont(Theme::font(10.5f,true));
  g.drawFittedText(processor.presetName,283,20,113,19,juce::Justification::centred,1);
 }
}
void FooterBar::mouseUp(const juce::MouseEvent& e)
{
 const auto p=logical(e.position);
 if(p.x<195.f)return;
 if(p.x<257.f){if(p.y<=20.f)menu();else rename();return;}
 if(p.y<17.f)return;
 if(p.x>=258.f&&p.x<=283.f){
  applyPreset((preset+ScratchPresets::count()-1)%ScratchPresets::count());
  if(loadPreset)loadPreset(preset);
 }else if(p.x>=398.f){
  applyPreset((preset+1)%ScratchPresets::count());
  if(loadPreset)loadPreset(preset);
 }else if(p.x>=284.f&&p.x<397.f)rename();
}
void FooterBar::applyPreset(int index)
{
 preset=juce::jlimit(0,ScratchPresets::count()-1,index);
 processor.setCurve(ScratchPresets::make(preset));
 processor.presetName=ScratchPresets::names[(size_t)preset];
 repaint();
}
void FooterBar::rename()
{
 if(renaming)return;
 renaming=true;
 nameEditor.setText(processor.presetName,juce::dontSendNotification);
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
  if(name.isNotEmpty())processor.presetName=name;
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
   if(safe->loadPreset)safe->loadPreset(command-100);
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
   safe->processor.setCurve({{0.f,1.f,0.f},{1.f,0.f,0.f}});
   safe->processor.presetName="Init";
   if(safe->loadPreset)safe->loadPreset(-1);
   safe->repaint();return;
  }
  if(command==4){
   juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"ScratchPad",
    "Skelly Neon v1.7. Draw dots, drag diamonds to bend, Alt to disable snap, Ctrl+wheel zoom, Ctrl+Z undo. STOP can be automated in your DAW.");
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
     file.withFileExtension(".scratchpad").replaceWithData(data.getData(),data.getSize());
    }else{
     juce::MemoryBlock data;
     if(file.loadFileAsData(data))safe->processor.setStateInformation(data.getData(),(int)data.getSize());
     if(safe->loadPreset)safe->loadPreset(-1);
    }
    safe->repaint();
   });
  }
 });
}
