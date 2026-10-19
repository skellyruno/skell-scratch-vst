#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>
namespace {
std::vector<CurveNode> initialCurve() { return {{0,1.0f,0},{.025f,-1,0},{.05f,1.1f,0},{.075f,-1,0},{.10f,1,0},{.125f,-1,0},{.15f,1,0},{.175f,-1,0},{.20f,0,0},{.25f,-1,0},{.32f,1,0},{.63f,.7f,-.12f},{1,0,0}}; }
float hermite(float a,float b,float c,float d,float t){ const float c0=b,c1=.5f*(c-a),c2=a-2.5f*b+2*c-.5f*d,c3=.5f*(d-a)+1.5f*(b-c); return ((c3*t+c2)*t+c1)*t+c0; }
}
ScratchPadAudioProcessor::ScratchPadAudioProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),apvts(*this,nullptr,"Parameters",makeParameters()) { setCurve(initialCurve()); }
juce::AudioProcessorValueTreeState::ParameterLayout ScratchPadAudioProcessor::makeParameters(){
 juce::AudioProcessorValueTreeState::ParameterLayout p;
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"scratchAmount",1},"Scratch Amount",juce::NormalisableRange<float>(0.0f,100.0f,1.0f),100.0f));
 p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"stopTrigger",1},"Stop",false));
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"loopLength",1},"Loop Length",juce::StringArray{"1/4","1/2","1 bar","2 bars","4 bars"},2));
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"speedRange",1},"Speed Range",juce::StringArray{"+/-1x","+/-2x","+/-4x"},1));
 p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"sync",1},"Tempo Sync",true)); return p;
}
void ScratchPadAudioProcessor::prepareToPlay(double sr,int block){sampleRate=std::max(1.0,sr); ring.setSize(std::max(1,getTotalNumInputChannels()),(int)std::ceil(sr*60.0/20.0*4*4)+block+8); ring.clear();writePos=0;readPos=0;phaseSamples=0; loopSamples=std::min(ring.getNumSamples()-8,(int)(sr*2));samplesPerCycle=loopSamples; fadeSamples=std::max(1,(int)std::round(sr*.002));fadeRemaining=0;previousOutput.assign((size_t)ring.getNumChannels(),0);amount.reset(sr,.015);amount.setCurrentAndTargetValue(1);setLatencySamples(0);}
void ScratchPadAudioProcessor::setCurve(std::vector<CurveNode> nodes){for(auto& n:nodes){n.x=juce::jlimit(0.f,1.f,n.x);n.y=juce::jlimit(-2.f,2.f,n.y);n.tension=juce::jlimit(-2.f,2.f,n.tension);}std::stable_sort(nodes.begin(),nodes.end(),[](auto&a,auto&b){return a.x<b.x;});if(nodes.empty())nodes={{0,0,0},{1,0,0}};auto ptr=std::make_shared<const ScratchCurveData>(ScratchCurveData{std::move(nodes)});std::atomic_store(&curve,std::move(ptr));}
float ScratchPadAudioProcessor::evaluate(const ScratchCurveData& c,float x){auto& v=c.nodes;if(v.empty())return 0;if(x<=v.front().x)return v.front().y;if(x>=v.back().x)return v.back().y;auto it=std::upper_bound(v.begin(),v.end(),x,[](float t,const CurveNode& n){return t<n.x;});size_t i=(size_t)(it-v.begin()-1);auto&a=v[i];auto&b=v[i+1];float t=(x-a.x)/std::max(.00001f,b.x-a.x);return juce::jlimit(-2.f,2.f,a.y+(b.y-a.y)*t+a.tension*4*t*(1-t));}
float ScratchPadAudioProcessor::interpolate(int ch,double pos) const {int n=ring.getNumSamples();int i=(int)std::floor(pos);float t=(float)(pos-i);auto at=[&](int idx){idx%=n;if(idx<0)idx+=n;return ring.getSample(ch,idx);};return hermite(at(i-1),at(i),at(i+1),at(i+2),t);}
void ScratchPadAudioProcessor::processBlock(juce::AudioBuffer<float>& buf,juce::MidiBuffer&){juce::ScopedNoDenormals guard;if(ring.getNumSamples()<8){buf.clear();return;}
 auto* a=apvts.getRawParameterValue("scratchAmount");auto* len=apvts.getRawParameterValue("loopLength");auto* range=apvts.getRawParameterValue("speedRange");auto* sync=apvts.getRawParameterValue("sync");auto* stop=apvts.getRawParameterValue("stopTrigger");
 double tempo=120;if(auto* ph=getPlayHead()){if(auto info=ph->getPosition()){if(auto v=info->getBpm())tempo=juce::jlimit(20.,400.,*v);}}bpm.store(tempo);double beat=60.0/(sync->load()>.5f?tempo:120.0);static constexpr double bars[]={.25,.5,1,2,4};int requested=juce::jlimit(8,ring.getNumSamples()-8,(int)std::round(sampleRate*beat*4*bars[juce::jlimit(0,4,(int)len->load())]));
 if(requested!=loopSamples){loopSamples=requested;phaseSamples=0;readPos=writePos;fadeRemaining=fadeSamples;}
 amount.setTargetValue(a->load()*.01f); auto c=curveSnapshot();int nc=buf.getNumChannels();int rn=ring.getNumSamples();int channels=std::min(nc,ring.getNumChannels());bool held=stop->load()>.5f||stopping.load();if(held&&!wasStopped)fadeRemaining=fadeSamples;wasStopped=held;
 for(int i=0;i<buf.getNumSamples();++i){for(int ch=0;ch<channels;++ch)ring.setSample(ch,writePos,buf.getSample(ch,i));
 float position=(float)phaseSamples/std::max(1,loopSamples);float speed=held?0:evaluate(*c,position)*amount.getNextValue()*(range->load()==0?1.f:range->load()==1?2.f:4.f)*.5f;
 double newRead=readPos+speed;while(newRead<0)newRead+=rn;while(newRead>=rn)newRead-=rn;
 if(std::abs(newRead-readPos)>rn*.5)fadeRemaining=fadeSamples;
 for(int ch=0;ch<channels;++ch){float wet=interpolate(ch,readPos);if(fadeRemaining>0){float mix=1.f-(float)fadeRemaining/fadeSamples;wet=previousOutput[(size_t)ch]*(1-mix)+wet*mix;}buf.setSample(ch,i,wet);previousOutput[(size_t)ch]=wet;}
 for(int ch=channels;ch<nc;++ch)buf.clear(ch,i,1);
 if(fadeRemaining>0)--fadeRemaining;readPos=newRead;writePos=(writePos+1)%rn;
 if(!held){if(++phaseSamples>=loopSamples){phaseSamples=0;fadeRemaining=fadeSamples;}} }
 playhead.store((float)phaseSamples/std::max(1,loopSamples));}
void ScratchPadAudioProcessor::getStateInformation(juce::MemoryBlock& dest){auto root=apvts.copyState();juce::ValueTree curveTree("CURVE");for(const auto& n:curveSnapshot()->nodes){juce::ValueTree v("NODE");v.setProperty("x",n.x,nullptr);v.setProperty("y",n.y,nullptr);v.setProperty("tension",n.tension,nullptr);curveTree.addChild(v,-1,nullptr);}root.addChild(curveTree,-1,nullptr);root.setProperty("presetName",presetName,nullptr);if(auto xml=root.createXml())copyXmlToBinary(*xml,dest);}
void ScratchPadAudioProcessor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size)){auto root=juce::ValueTree::fromXml(*xml);if(root.isValid()){auto nodes=root.getChildWithName("CURVE");if(nodes.isValid()){std::vector<CurveNode> out;for(auto node:nodes)if(node.hasType("NODE"))out.push_back({(float)node.getProperty("x"),(float)node.getProperty("y"),(float)node.getProperty("tension")});if(!out.empty())setCurve(std::move(out));root.removeChild(nodes,nullptr);}presetName=root.getProperty("presetName","scratch it!").toString();apvts.replaceState(root);}}}
juce::AudioProcessorEditor* ScratchPadAudioProcessor::createEditor(){return new ScratchPadAudioProcessorEditor(*this);}

// JUCE plugin client entry point, required by both VST3 and Standalone targets.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ScratchPadAudioProcessor();
}
