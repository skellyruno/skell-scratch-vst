#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "HeadSafety.h"
#include "DSPHelpers.h"
#include "LoopTiming.h"
#include <cmath>
#include <algorithm>
namespace {
std::vector<CurveNode> initialCurve() { return {{0,1.0f,0},{.025f,-1,0},{.05f,1.1f,0},{.075f,-1,0},{.10f,1,0},{.125f,-1,0},{.15f,1,0},{.175f,-1,0},{.20f,0,0},{.25f,-1,0},{.32f,1,0},{.63f,.7f,-.12f},{1,0,0}}; }
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
bool ScratchPadAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
 const auto mainIn=layout.getMainInputChannelSet();
 const auto mainOut=layout.getMainOutputChannelSet();
 return (mainIn==juce::AudioChannelSet::mono() ||
         mainIn==juce::AudioChannelSet::stereo()) && mainIn==mainOut;
}

void ScratchPadAudioProcessor::prepareToPlay(double sr, int block)
{
 sampleRate=std::max(1.0,sr);
 // 4 bars at 20 BPM + a guard region. Allocated on prepare, never on audio callback.
 const auto capacity=(int)std::ceil(sampleRate*60.0/20.0*16.0)+std::max(1,block)+8;
 ring.setSize(std::max(1,getTotalNumInputChannels()),capacity);
 ring.clear();
 writePos=0;readPos=0;transitionReadPos=0;recordedSamples=0;
 captureReady=false;
 phaseSamples=0;loopSamples=std::min(capacity-8,(int)(sampleRate*2.0));
 fadeSamples=std::max(1,(int)std::round(sampleRate*.002));
 transitionRemaining=0;transitionActive=false;
 lastHostPlaying=false;hostTransportKnown=false;expectedHostSample=0;
 amount.reset(sampleRate,.015);
 speedRangeSmoothing.reset(sampleRate,.012);
 speedSlew.prepare(sampleRate, .0004);
 speedSlew.reset();
 { const auto raw=apvts.getRawParameterValue("speedRange")->load();
   speedRangeSmoothing.setCurrentAndTargetValue(raw<.5f?1.f:raw<1.5f?2.f:4.f); }
 amount.setCurrentAndTargetValue(apvts.getRawParameterValue("scratchAmount")->load()*.01f);
 stopGain.reset(sampleRate,.002);
 captureGain.reset(sampleRate,.002);
 captureGain.setCurrentAndTargetValue(0.f);
 stopGain.setCurrentAndTargetValue(
     apvts.getRawParameterValue("stopTrigger")->load() > .5f ? 0.f : 1.f);
 currentSpeed.store(0.f, std::memory_order_relaxed);
 captureProgress.store(0.f, std::memory_order_relaxed);
 setLatencySamples(0);
}
void ScratchPadAudioProcessor::setCurve(std::vector<CurveNode> nodes)
{
 CurveMath::normalize(nodes);
 std::lock_guard<std::mutex> publishLock(curvePublishMutex);
 auto updated = std::make_shared<const ScratchCurveData>(ScratchCurveData{std::move(nodes)});
 // Publish without any allocation, lock or reference counting on the audio thread.
 // Old snapshots are reclaimed only when no audio reader is active.
 auto previous = std::atomic_exchange(&curve, updated);
 realtimeCurve.store(updated.get(), std::memory_order_seq_cst);
 if (previous) retiredCurves.push_back(std::move(previous));
 if (activeAudioReaders.load(std::memory_order_seq_cst) == 0) retiredCurves.clear();
 curveVersion.fetch_add(1, std::memory_order_relaxed);
}
void ScratchPadAudioProcessor::collectRetiredCurves()
{
 // No locks or destruction in the audio callback; the UI timer reclaims
 // abandoned edits when a gap between process blocks is available.
 std::lock_guard<std::mutex> publishLock(curvePublishMutex);
 if (activeAudioReaders.load(std::memory_order_seq_cst) == 0)
     retiredCurves.clear();
}
float ScratchPadAudioProcessor::interpolate(int ch,double pos) const
{
 const int n=ring.getNumSamples();
 const auto integer=(int)std::floor(pos);
 const float t=(float)(pos-integer);
 auto at=[&](int idx)->float
 {
  if(idx<0)idx+=n;
  else if(idx>=n)idx-=n;
  // An interpolator tap may precede the first recorded sample. Never
  // synthesize from unrecorded buffer memory or pre-roll zeros indefinitely.
  const int age=writePos>=idx?writePos-idx:writePos-idx+n;
  if(age>=recordedSamples) return 0.f;
  return ring.getSample(ch,idx);
 };
 return ScratchDSP::hermiteBounded(at(integer-1),at(integer),
                                   at(integer+1),at(integer+2),t);
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
 bool hostPpqKnown=false;
 double hostPpq=0.0;
 if(auto* ph=getPlayHead())
  if(auto info=ph->getPosition())
  {
   if(auto v=info->getBpm())
    if(std::isfinite(*v)&&*v>0.0)tempo=juce::jlimit(20.0,400.0,*v);
   hostPlaying=info->getIsPlaying();
   if (auto ppq=info->getPpqPosition()) {
    if (std::isfinite(*ppq)) {hostPpq=*ppq;hostPpqKnown=true;}
   }
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
 const int lengthChoice=juce::jlimit(0,4,(int)lengthParam->load());
 const int requested=juce::jlimit(8,ringSize-8,
  (int)std::round(sampleRate*beat*4.0*bars[lengthChoice]));
 if(requested!=loopSamples)
 {
  const int previousLoopLength=loopSamples;
  const double progress=(double)phaseSamples/std::max(1,loopSamples);
  loopSamples=requested;
  phaseSamples=juce::jlimit(0,loopSamples-1,(int)(progress*loopSamples));
  // Small ongoing tempo ramps adjust envelope timing without retriggering
  // a seek fade every audio block. Only meaningful loop-size jumps retarget.
  if (std::abs(requested-previousLoopLength)>std::max(32,fadeSamples*4))
  {
   transitionReadPos=readPos;
   readPos=(double)((writePos-loopSamples+ringSize)%ringSize);
   transitionRemaining=fadeSamples;
   transitionActive=true;
  }
 }
 if(hostDiscontinuity && loopSamples>0)
 {
  // Resume at the correct musical point when the host supplies PPQ.
  // For unsynced mode or missing PPQ, restart at the beginning.
  phaseSamples=(syncParam->load()>.5f && hostPpqKnown)
      ? ScratchTiming::phaseFromPpq(hostPpq,loopSamples,lengthChoice) : 0;
  transitionReadPos=readPos;
  readPos=(double)((writePos-loopSamples+ringSize)%ringSize);
  transitionRemaining=fadeSamples;
  transitionActive=true;
 }
 amount.setTargetValue(amountParam->load()*.01f);
 const float requestedRange=rangeParam->load()<.5f?1.f:rangeParam->load()<1.5f?2.f:4.f;
 speedRangeSmoothing.setTargetValue(requestedRange);
 // The seq_cst reader guard prevents reclamation of the snapshot during
 // this block, including a concurrent editor edit.
 activeAudioReaders.fetch_add(1,std::memory_order_seq_cst);
 const auto* curveData=realtimeCurve.load(std::memory_order_seq_cst);
 CurveMath::MonotonicCursor curveCursor(curveData ? &curveData->nodes : nullptr);
 float latestSpeed = 0.f;
 const int channels=std::min(buf.getNumChannels(),ring.getNumChannels());
 const bool stopped=stopParam->load()>.5f;
 stopGain.setTargetValue(stopped?0.f:1.f);
 for(int i=0;i<buf.getNumSamples();++i)
 {
  // Capture input before replacing it with the processed signal.
  for(int ch=0;ch<channels;++ch) {
   const float input=buf.getSample(ch,i);
   ring.setSample(ch,writePos,std::isfinite(input)?input:0.f);
  }
  recordedSamples=std::min(recordedSamples+1,ringSize);
  const bool fullLoop=HeadSafety::hasFullCapture(recordedSamples,loopSamples);
  if(fullLoop != captureReady)
  {
   captureReady=fullLoop;
   if(captureReady)
   {
    // The previous head might still point into unrecorded space. This
    // retarget happens while the wet signal is silent, before the 2 ms fade.
    readPos=HeadSafety::capturedLoopStart(writePos,loopSamples,ringSize);
    transitionReadPos=readPos;
    transitionRemaining=0;
    transitionActive=false;
   }
   captureGain.setTargetValue(captureReady?1.f:0.f);
  }
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
  const float speedMultiplier=speedRangeSmoothing.getNextValue();
  const float targetSpeed=curveCursor.value(x)*smoothedAmount*speedMultiplier*.5f;
  const float speed=stopped?0.f:speedSlew.process(targetSpeed);
  if (stopped) speedSlew.reset();
  latestSpeed=speed;
  const float gain=stopGain.getNextValue();
  const float wetAmount=smoothedAmount*captureGain.getNextValue();
  const bool needWet=wetAmount>0.000001f;
  const float crossfade=transitionActive
    ?1.f-(float)transitionRemaining/(float)fadeSamples:1.f;
  // Equal-gain, smoothstep crossfade: unlike equal-power blending, this
  // avoids a +3 dB boost when both heads carry strongly correlated audio.
  const float blend=ScratchDSP::smoothstep(crossfade);
  const float oldGain=transitionActive?1.f-blend:0.f;
  const float newGain=transitionActive?blend:1.f;
  for(int ch=0;ch<channels;++ch)
  {
   const float dry=ring.getSample(ch,writePos);
   // Skip interpolator taps entirely while warming up or at 0% amount.
   // This also guarantees exact, zero-latency dry signal before capture.
   float output=dry;
   if(needWet)
   {
    const float newest=interpolate(ch,readPos);
    const float older=transitionActive?interpolate(ch,transitionReadPos):0.f;
    const float wet=older*oldGain+newest*newGain;
    output=ScratchDSP::mixDryWet(dry,wet,wetAmount);
   }
   output*=gain;
   buf.setSample(ch,i,std::isfinite(output)?output:0.f);
  }
  for(int ch=channels;ch<buf.getNumChannels();++ch)buf.clear(ch,i,1);
  readPos=HeadSafety::advance(readPos,speed,ringSize);
  if(transitionActive)
  {
   transitionReadPos=HeadSafety::advance(transitionReadPos,speed,ringSize);
   if(--transitionRemaining<=0)transitionActive=false;
  }
  if (++writePos >= ringSize) writePos=0;
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
 currentSpeed.store(latestSpeed,std::memory_order_relaxed);
 captureProgress.store(juce::jlimit(0.f,1.f,
     (float)recordedSamples/std::max(1,loopSamples)),std::memory_order_relaxed);
 activeAudioReaders.fetch_sub(1,std::memory_order_seq_cst);
}
juce::String ScratchPadAudioProcessor::getPresetName() const
{
 std::lock_guard<std::mutex> lock(presetNameMutex);
 return presetName;
}
void ScratchPadAudioProcessor::setPresetName(const juce::String& name)
{
 std::lock_guard<std::mutex> lock(presetNameMutex);
 presetName = name.substring(0, 48);
}

void ScratchPadAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
 auto root = apvts.copyState();
 juce::ValueTree curveTree("CURVE");
 if (const auto snapshot = curveSnapshot()) {
  for (const auto& node : snapshot->nodes) {
   juce::ValueTree entry("NODE");
   entry.setProperty("x", node.x, nullptr);
   entry.setProperty("y", node.y, nullptr);
   entry.setProperty("tension", node.tension, nullptr);
   curveTree.addChild(entry, -1, nullptr);
  }
 }
 root.addChild(curveTree, -1, nullptr);
 root.setProperty("presetName", getPresetName(), nullptr);
 if (const auto xml = root.createXml())
  copyXmlToBinary(*xml, destination);
}

void ScratchPadAudioProcessor::setStateInformation(const void* data, int size)
{
 if (data == nullptr || size <= 0) return;
 const auto xml = getXmlFromBinary(data, size);
 if (xml == nullptr) return;
 auto root = juce::ValueTree::fromXml(*xml);
 if (!root.isValid() || root.getType() != apvts.state.getType()) return;

 if (auto curveTree = root.getChildWithName("CURVE"); curveTree.isValid()) {
  std::vector<CurveNode> nodes;
  nodes.reserve(static_cast<std::size_t>(std::min(512, curveTree.getNumChildren())));
  for (auto entry : curveTree) {
   if (!entry.hasType("NODE")) continue;
   nodes.push_back({static_cast<float>(entry.getProperty("x")),
                    static_cast<float>(entry.getProperty("y")),
                    static_cast<float>(entry.getProperty("tension"))});
   if (nodes.size() >= 512) break;
  }
  if (!nodes.empty()) setCurve(std::move(nodes));
  root.removeChild(curveTree, nullptr);
 }
 setPresetName(root.getProperty("presetName", "scratch it!").toString());
 apvts.replaceState(root);
}

juce::AudioProcessorEditor* ScratchPadAudioProcessor::createEditor(){return new ScratchPadAudioProcessorEditor(*this);}

// JUCE plugin client entry point, required by both VST3 and Standalone targets.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ScratchPadAudioProcessor();
}
