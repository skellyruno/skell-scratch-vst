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
 bool isBusesLayoutSupported(const BusesLayout&) const override;
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
 std::atomic<float> playhead{0.f};
 // UI-only meters: updated once per processed block, not every sample.
 std::atomic<float> currentSpeed{0.f};
 std::atomic<float> captureProgress{0.f};
 std::atomic<unsigned> curveVersion{0};
 // Called only by UI/non-real-time threads; never inside processBlock.
 void collectRetiredCurves();
 std::atomic<double> bpm{120.0};
 juce::String getPresetName() const;
 void setPresetName(const juce::String& name);
private:
 // The UI owns snapshots. Audio uses a non-owning pointer held alive until
 // all audio blocks that could have seen it have completed.
 std::shared_ptr<const ScratchCurveData> curve;
 std::atomic<const ScratchCurveData*> realtimeCurve{nullptr};
 std::atomic<unsigned> activeAudioReaders{0};
 std::vector<std::shared_ptr<const ScratchCurveData>> retiredCurves;
 std::mutex curvePublishMutex;  // Non-RT editors/state restores only; audio never locks.
 mutable std::mutex presetNameMutex;
 juce::String presetName{"scratch it!"};
 juce::AudioBuffer<float> ring;
 int writePos=0, loopSamples=1, phaseSamples=0;
 double readPos=0, transitionReadPos=0, sampleRate=44100;
 int recordedSamples=0, transitionRemaining=0;
 bool lastHostPlaying=false;
 bool hostTransportKnown=false;
 juce::int64 expectedHostSample=0;
 bool transitionActive=false;
 int fadeSamples=1;
 juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> stopGain;
 juce::SmoothedValue<float> amount;
 juce::SmoothedValue<float> speedRangeSmoothing;
 float interpolate(int channel,double pos) const;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScratchPadAudioProcessor)
};
