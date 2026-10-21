#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "HeadSafety.h"
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
void ScratchPadAudioProcessor::prepareToPlay(double sr, int block)
{
 sampleRate=std::max(1.0,sr);
 // 4 bars at 20 BPM + a guard region. Allocated on prepare, never on audio callback.
 const auto capacity=(int)std::ceil(sampleRate*60.0/20.0*16.0)+std::max(1,block)+8;
 ring.setSize(std::max(1,getTotalNumInputChannels()),capacity);
 ring.clear();
 writePos=0;readPos=0;transitionReadPos=0;recordedSamples=0;
 phaseSamples=0;loopSamples=std::min(capacity-8,(int)(sampleRate*2.0));
 samplesPerCycle=loopSamples;
 fadeSamples=std::max(1,(int)std::round(sampleRate*.002));
 transitionRemaining=0;transitionActive=false;wasStopped=false;
 lastHostPlaying=false;hostTransportKnown=false;expectedHostSample=0;
 amount.reset(sampleRate,.015);
 amount.setCurrentAndTargetValue(apvts.getRawParameterValue("scratchAmount")->load()*.01f);
 stopGain.reset(sampleRate,.002);
 stopGain.setCurrentAndTargetValue(1.0f);
 setLatencySamples(0);
}
void ScratchPadAudioProcessor::setCurve(std::vector<CurveNode> nodes){if(nodes.size()>512)nodes.resize(512);for(auto& n:nodes){if(!std::isfinite(n.x))n.x=0; if(!std::isfinite(n.y))n.y=0; if(!std::isfinite(n.tension))n.tension=0; n.x=juce::jlimit(0.f,1.f,n.x);n.y=juce::jlimit(-2.f,2.f,n.y);n.tension=juce::jlimit(-2.f,2.f,n.tension);}std::stable_sort(nodes.begin(),nodes.end(),[](auto&a,auto&b){return a.x<b.x;});
 if(nodes.empty())nodes={{0,0,0},{1,0,0}};
 // Remove duplicate x positions produced by grid snapping or preset stretch.
 // Keep the latest node at a given time.
 std::vector<CurveNode> cleaned;
 cleaned.reserve(nodes.size()+2);
 for(auto n:nodes)
 {
  if(!cleaned.empty() && n.x-cleaned.back().x<0.0001f)
   cleaned.back()=n;
  else cleaned.push_back(n);
 }
 if(cleaned.size()==1)
 {
  if(cleaned.front().x>0.f)cleaned.insert(cleaned.begin(),CurveNode{0.f,cleaned.front().y,0.f});
  else cleaned.push_back(CurveNode{1.f,cleaned.front().y,0.f});
 }
 nodes=std::move(cleaned);auto ptr=std::make_shared<const ScratchCurveData>(ScratchCurveData{std::move(nodes)});
 // Publish the new snapshot before retiring the old one. The audio thread
 // never increments/decrements shared_ptr refcounts or frees heap memory.
 auto old=std::atomic_exchange(&curve,ptr);
 realtimeCurve.store(ptr.get(),std::memory_order_seq_cst);
 if(old) retiredCurves.push_back(std::move(old));
 // Reclamation happens only on the editor/message thread.
 if(activeAudioReaders.load(std::memory_order_seq_cst)==0) retiredCurves.clear();}
float ScratchPadAudioProcessor::evaluate(const ScratchCurveData& c,float x){auto& v=c.nodes;if(v.empty())return 0;if(x<=v.front().x)return v.front().y;if(x>=v.back().x)return v.back().y;auto it=std::upper_bound(v.begin(),v.end(),x,[](float t,const CurveNode& n){return t<n.x;});size_t i=(size_t)(it-v.begin()-1);auto&a=v[i];auto&b=v[i+1];float t=(x-a.x)/std::max(.00001f,b.x-a.x);return juce::jlimit(-2.f,2.f,a.y+(b.y-a.y)*t+a.tension*4*t*(1-t));}
float ScratchPadAudioProcessor::interpolate(int ch,double pos) const
{
 const int n=ring.getNumSamples();
 const auto integer=(int)std::floor(pos);
 const float t=(float)(pos-integer);
 auto at=[&](int idx)->float
 {
  idx%=n; if(idx<0)idx+=n;
  // An interpolator tap may precede the first recorded sample. Never
  // synthesize from unrecorded buffer memory or pre-roll zeros indefinitely.
  const int age=(writePos-idx+n)%n;
  if(age>=recordedSamples) return 0.f;
  return ring.getSample(ch,idx);
 };
 return hermite(at(integer-1),at(integer),at(integer+1),at(integer+2),t);
}
void ScratchPadAudioProcessor::processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
 juce::ScopedNoDenormals guard;
 const int ringSize=ring.getNumSamples();
 if(ringSize<8){buf.clear();return;}
 const auto* amountParam=apvts.getRawParameterValue("scratchAmount");
 const auto* lengthParam=apvts.getRawParameterValue("loopLength");
 const auto* rangeParam=apvts.getRawParameterValue("speedRange");
 const auto* syncParam=apvts.getRawParameterValue("sync");
 const auto* stopParam=apvts.getRawParameterValue("stopTrigger");
 double tempo=120.0;
 bool hostPlaying=false;
 bool hostStateAvailable=false;
 bool hostDiscontinuity=false;
 if(auto* ph=getPlayHead())
  if(auto info=ph->getPosition())
  {
   if(auto v=info->getBpm())
    if(std::isfinite(*v)&&*v>0.0)tempo=juce::jlimit(20.0,400.0,*v);
   hostPlaying=info->getIsPlaying();
   hostStateAvailable=true;
   if(auto sample=info->getTimeInSamples())
   {
    if(hostTransportKnown && hostPlaying && lastHostPlaying
       && std::abs(*sample-expectedHostSample)>juce::jmax<juce::int64>(64,static_cast<juce::int64>(buf.getNumSamples())*2))
     hostDiscontinuity=true;
    expectedHostSample=*sample+buf.getNumSamples();
    hostTransportKnown=true;
   }
   else hostTransportKnown=false;
  }
 // A timeline seek restarts the envelope, but the audio read jump is faded.
 // Do not reset the recording buffer: it contains the live audio history.
 if(hostStateAvailable && hostPlaying && !lastHostPlaying)
  hostDiscontinuity=true;
 if(hostStateAvailable) lastHostPlaying=hostPlaying;
 bpm.store(tempo,std::memory_order_relaxed);
 static constexpr double bars[]={.25,.5,1.0,2.0,4.0};
 const double beat=60.0/(syncParam->load()>.5f?tempo:120.0);
 const int requested=juce::jlimit(8,ringSize-8,
  (int)std::round(sampleRate*beat*4.0*bars[juce::jlimit(0,4,(int)lengthParam->load())]));
 if(requested!=loopSamples)
 {
  const double progress=(double)phaseSamples/std::max(1,loopSamples);
  loopSamples=requested;
  phaseSamples=juce::jlimit(0,loopSamples-1,(int)(progress*loopSamples));
  // Switch to the new historical region through two simultaneous read heads.
  transitionReadPos=readPos;
  readPos=(double)((writePos-loopSamples+ringSize)%ringSize);
  transitionRemaining=fadeSamples;
  transitionActive=true;
 }
 if(hostDiscontinuity && loopSamples>0)
 {
  phaseSamples=0;
  transitionReadPos=readPos;
  readPos=(double)((writePos-loopSamples+ringSize)%ringSize);
  transitionRemaining=fadeSamples;
  transitionActive=true;
 }
 amount.setTargetValue(amountParam->load()*.01f);
 // The seq_cst reader guard prevents reclamation of the snapshot during
 // this block, including a concurrent editor edit.
 activeAudioReaders.fetch_add(1,std::memory_order_seq_cst);
 const auto* curveData=realtimeCurve.load(std::memory_order_seq_cst);
 const int channels=std::min(buf.getNumChannels(),ring.getNumChannels());
 const float speedMultiplier=rangeParam->load()<.5f?1.f:rangeParam->load()<1.5f?2.f:4.f;
 const bool stopped=stopParam->load()>.5f||stopping.load(std::memory_order_relaxed);
 stopGain.setTargetValue(stopped?0.f:1.f);
 for(int i=0;i<buf.getNumSamples();++i)
 {
  // Capture input before replacing it with the processed signal.
  for(int ch=0;ch<channels;++ch)
   ring.setSample(ch,writePos,buf.getSample(ch,i));
  recordedSamples=std::min(recordedSamples+1,ringSize);
  // Reposition before the writer catches a frozen/slow/reversing read head.
  // The guard is larger than the fade, so the old head remains readable
  // while the new head fades in. Avoid retriggering during an active fade.
  const int guardSamples=std::max(8,fadeSamples*6+8);
  if (!transitionActive && recordedSamples > guardSamples*8
      && HeadSafety::approachingOverwrite(writePos,readPos,ringSize,guardSamples))
  {
   transitionReadPos=readPos;
   readPos=HeadSafety::safePosition(writePos,ringSize,
    std::min(loopSamples,recordedSamples-guardSamples*2),guardSamples);
   transitionRemaining=fadeSamples;
   transitionActive=true;
  }
  const float x=(float)phaseSamples/std::max(1,loopSamples);
  const float smoothedAmount=amount.getNextValue();
  const float speed=stopped||curveData==nullptr?0.f:evaluate(*curveData,x)*smoothedAmount*speedMultiplier*.5f;
  const float gain=stopGain.getNextValue();
  const float crossfade=transitionActive
    ?1.f-(float)transitionRemaining/(float)fadeSamples:1.f;
  // Equal-gain, smoothstep crossfade: unlike equal-power blending, this
  // avoids a +3 dB boost when both heads carry strongly correlated audio.
  const float blend=crossfade*crossfade*(3.f-2.f*crossfade);
  const float oldGain=transitionActive?1.f-blend:0.f;
  const float newGain=transitionActive?blend:1.f;
  for(int ch=0;ch<channels;++ch)
  {
   const float newest=interpolate(ch,readPos);
   const float older=transitionActive?interpolate(ch,transitionReadPos):0.f;
   const float wet=(older*oldGain+newest*newGain)*gain;
   // Read heads start within unrecorded history during initial capture.
   buf.setSample(ch,i,recordedSamples<4||!std::isfinite(wet)?0.f:wet);
  }
  for(int ch=channels;ch<buf.getNumChannels();++ch)buf.clear(ch,i,1);
  readPos=HeadSafety::wrap(readPos+speed,ringSize);
  if(transitionActive)
  {
   transitionReadPos=HeadSafety::wrap(transitionReadPos+speed,ringSize);
   if(--transitionRemaining<=0)transitionActive=false;
  }
  writePos=(writePos+1)%ringSize;
  if(!stopped && ++phaseSamples>=loopSamples)
  {
   phaseSamples=0;
   transitionReadPos=readPos;
   readPos=(double)((writePos-loopSamples+ringSize)%ringSize);
   transitionRemaining=fadeSamples;
   transitionActive=true;
  }
 }
 playhead.store((float)phaseSamples/std::max(1,loopSamples),std::memory_order_relaxed);
 activeAudioReaders.fetch_sub(1,std::memory_order_seq_cst);
}
void ScratchPadAudioProcessor::getStateInformation(juce::MemoryBlock& dest){auto root=apvts.copyState();juce::ValueTree curveTree("CURVE");for(const auto& n:curveSnapshot()->nodes){juce::ValueTree v("NODE");v.setProperty("x",n.x,nullptr);v.setProperty("y",n.y,nullptr);v.setProperty("tension",n.tension,nullptr);curveTree.addChild(v,-1,nullptr);}root.addChild(curveTree,-1,nullptr);root.setProperty("presetName",presetName,nullptr);if(auto xml=root.createXml())copyXmlToBinary(*xml,dest);}
void ScratchPadAudioProcessor::setStateInformation(const void* data,int size){if(auto xml=getXmlFromBinary(data,size)){auto root=juce::ValueTree::fromXml(*xml);if(root.isValid()){auto nodes=root.getChildWithName("CURVE");if(nodes.isValid()){std::vector<CurveNode> out;for(auto node:nodes)if(node.hasType("NODE"))out.push_back({(float)node.getProperty("x"),(float)node.getProperty("y"),(float)node.getProperty("tension")});if(!out.empty())setCurve(std::move(out));root.removeChild(nodes,nullptr);}presetName=root.getProperty("presetName","scratch it!").toString();apvts.replaceState(root);}}}
juce::AudioProcessorEditor* ScratchPadAudioProcessor::createEditor(){return new ScratchPadAudioProcessorEditor(*this);}

// JUCE plugin client entry point, required by both VST3 and Standalone targets.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ScratchPadAudioProcessor();
}
