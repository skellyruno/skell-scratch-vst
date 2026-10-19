#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include <memory>
struct CurveNode { float x=0,y=0,tension=0; };
struct CurveData { std::vector<CurveNode> nodes; };
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
 std::shared_ptr<const CurveData> curveSnapshot() const { return std::atomic_load(&curve); }
 void setCurve(std::vector<CurveNode>);
 std::atomic<float> playhead{0};
 std::atomic<double> bpm{120.0};
 std::atomic<bool> stopping{false};
 juce::String presetName{"scratch it!"};
private:
 std::shared_ptr<const CurveData> curve;
 juce::AudioBuffer<float> ring;
 int writePos=0, loopSamples=1, samplesPerCycle=1, phaseSamples=0;
 double readPos=0, sampleRate=44100, lastBpm=120;
 bool wasStopped=false;
 int fadeRemaining=0, fadeSamples=1;
 std::vector<float> previousOutput;
 juce::SmoothedValue<float> amount;
 static float evaluate(const CurveData&,float);
 float interpolate(int channel,double pos) const;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScratchPadAudioProcessor)
};
