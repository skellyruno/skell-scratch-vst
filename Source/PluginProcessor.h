#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>
#include "CurveMath.h"
struct ScratchCurveData { std::vector<CurveNode> nodes; };
class ScratchPadAudioProcessor : public juce::AudioProcessor {
public:
 ScratchPadAudioProcessor();
 ~ScratchPadAudioProcessor() override = default;
 void prepareToPlay(double, int) override;
 void releaseResources() override {}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override { return true; }
 const juce::String getName() const override { return "ScratchPad"; }
 bool acceptsMidi() const override { return false; } bool producesMidi() const override { return false; }
 bool isMidiEffect() const override { return false; }
 double getTailLengthSeconds() const override { return 0; }
 int getNumPrograms() override { return 1; } int getCurrentProgram() override { return 0; }
 void setCurrentProgram(int) override {} const juce::String getProgramName(int) override { return {}; }
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 juce::AudioProcessorValueTreeState apvts;
 static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
 std::shared_ptr<const ScratchCurveData> curveSnapshot() const { return std::atomic_load(&curve); }
 void setCurve(std::vector<CurveNode>);
 std::atomic<float> playhead{0};
 std::atomic<double> bpm{120.0};
 std::atomic<bool> stopping{false};
 juce::String presetName{"scratch it!"};
private:
 // The UI owns snapshots. Audio uses a non-owning pointer held alive until
 // all audio blocks that could have seen it have completed.
 std::shared_ptr<const ScratchCurveData> curve;
 std::atomic<const ScratchCurveData*> realtimeCurve{nullptr};
 std::atomic<unsigned> activeAudioReaders{0};
 std::vector<std::shared_ptr<const ScratchCurveData>> retiredCurves;
 std::mutex curvePublishMutex;  // Non-RT editors/state restores only; audio never locks.
 juce::AudioBuffer<float> ring;
 int writePos=0, loopSamples=1, samplesPerCycle=1, phaseSamples=0;
 double readPos=0, transitionReadPos=0, sampleRate=44100;
 int recordedSamples=0, transitionRemaining=0;
 bool lastHostPlaying=false;
 bool hostTransportKnown=false;
 juce::int64 expectedHostSample=0;
 bool transitionActive=false;
 bool wasStopped=false;
 int fadeRemaining=0, fadeSamples=1;
 juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> stopGain;
 juce::SmoothedValue<float> amount;
 juce::SmoothedValue<float> speedRangeSmoothing;
 static float evaluate(const ScratchCurveData&,float);
 float interpolate(int channel,double pos) const;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScratchPadAudioProcessor)
};
