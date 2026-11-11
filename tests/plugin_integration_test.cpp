#include "../Source/PluginProcessor.h"
#include "TestCheck.h"
#include <JuceHeader.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>

namespace {
constexpr double pi = 3.1415926535897932384626433832795;
constexpr int frames = 128;
constexpr double sampleRate = 48000.0;

void setParameter(ScratchPadAudioProcessor& processor, const char* id, float actualValue) {
    auto* parameter = processor.apvts.getParameter(id);
    REQUIRE(parameter != nullptr);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(actualValue));
}

struct BlockRunner {
    ScratchPadAudioProcessor& processor;
    juce::AudioBuffer<float> buffer{2, frames};
    juce::MidiBuffer midi;
    std::int64_t samplePosition = 0;

    template<typename Generator>
    float run(Generator&& generator, bool checkDry = false) {
        float maxAbs = 0.f;
        for (int i = 0; i < frames; ++i) {
            const float a = generator(samplePosition + i, 0);
            const float b = generator(samplePosition + i, 1);
            buffer.setSample(0, i, a);
            buffer.setSample(1, i, b);
        }
        processor.processBlock(buffer, midi);
        for (int i = 0; i < frames; ++i) {
            for (int ch = 0; ch < 2; ++ch) {
                const float y = buffer.getSample(ch, i);
                REQUIRE(std::isfinite(y));
                if (checkDry) {
                    const float input = generator(samplePosition + i, ch);
                    REQUIRE(std::abs(y - input) < 0.000001f);
                }
                maxAbs = std::max(maxAbs, std::abs(y));
            }
        }
        samplePosition += frames;
        return maxAbs;
    }
};

void testActualAudio() {
    ScratchPadAudioProcessor processor;
    const auto initial = processor.curveSnapshot();
    REQUIRE(initial != nullptr && initial->nodes.size() == 8);
    REQUIRE(processor.getPresetName() == "Skelly Starter");
    setParameter(processor, "sync", 0.f);
    setParameter(processor, "loopLength", 0.f);
    setParameter(processor, "scratchAmount", 0.f);
    setParameter(processor, "stopTrigger", 0.f);
    setParameter(processor, "loopLock", 0.f);
    processor.prepareToPlay(sampleRate, frames);
    REQUIRE(processor.getLatencySamples() == 0);
    REQUIRE(!processor.hasReceivedAudio.load());
    BlockRunner runner{processor};
    const auto tone = [](std::int64_t i, int ch) {
        return static_cast<float>(.23 * std::sin(2 * pi * (ch == 0 ? 220. : 330.) * i / sampleRate));
    };
    // Tests the real processBlock, not just the mix helper. After 1s of
    // history at 0% depth output must remain bit-exact dry.
    for (int block = 0; block < 375; ++block) runner.run(tone, true);
    REQUIRE(processor.hasReceivedAudio.load());

    // New capture starts from the very next input sample. This proves that
    // silence fed later cannot overwrite the independently locked loop.
    setParameter(processor, "scratchAmount", 100.f);
    setParameter(processor, "loopLock", 1.f);
    // 1/4 bar at 120 BPM is one beat (0.5 s), NOT an eighth note.
    // Allow capture to finish according to musical loop length rather than
    // incorrectly assuming a fixed number of audio blocks.
    int captureBlocks = 0;
    while (processor.lockedStatus.load() != 2 && captureBlocks++ < 300)
        runner.run(tone);
    std::printf("JUCE Loop Lock: ready after %d blocks (%.3fs)\n",
                captureBlocks, captureBlocks * frames / sampleRate);
    REQUIRE(processor.lockedStatus.load() == 2);
    REQUIRE(processor.lockedProgress.load() == 1.f);
    const auto zero = [](std::int64_t, int) { return 0.f; };
    float maxPersistent = 0.f;
    for (int block = 0; block < 160; ++block)
        maxPersistent = std::max(maxPersistent, runner.run(zero));
    REQUIRE(maxPersistent > .015f);

    // Exercise the fully configured ±4x sinc path, not just the math helper.
    // Process at known amplitude and verify finite, bounded float output.
    setParameter(processor, "speedRange", 2.f);
    for (int block = 0; block < 220; ++block) {
        const float peak = runner.run(zero);
        REQUIRE(peak <= 1.f);
    }

    // Stop must mute the *actual* audio path after its 2ms smoothing ramp.
    setParameter(processor, "stopTrigger", 1.f);
    for (int block = 0; block < 8; ++block) runner.run(zero);
    REQUIRE(runner.run(zero) < .000001f);
    setParameter(processor, "stopTrigger", 0.f);
    for (int block = 0; block < 8; ++block) runner.run(zero);

    // Zero scratch amount is exact dry even when a capture is still locked.
    setParameter(processor, "scratchAmount", 0.f);
    for (int block = 0; block < 10; ++block) runner.run(zero);
    for (int block = 0; block < 20; ++block) runner.run(tone, true);

    // Damaged source samples must be sanitized before reaching the output.
    const auto broken = [](std::int64_t, int) { return std::numeric_limits<float>::quiet_NaN(); };
    for (int block = 0; block < 6; ++block) runner.run(broken);
    REQUIRE(processor.outputPeakHold.load(std::memory_order_relaxed)>0.f);
    const float held = ScratchDSP::consumePeak(processor.outputPeakHold);
    REQUIRE(held>0.f && processor.outputPeakHold.load()==0.f);
    processor.releaseResources();
    std::printf("Real JUCE DSP: dry passthrough, immutable capture, stop, release, NaN and latency passed\n");
}

// Stress the actual JUCE processBlock: full loop lock, ±4x read-head motion,
// rapid host automation and damaged/transient input at professional sample rates.
void testActualProcessorStress() {
 for (double rate : {44100.0,48000.0,96000.0,192000.0}) {
  ScratchPadAudioProcessor processor;
  setParameter(processor,"sync",0.f);
  setParameter(processor,"loopLength",0.f);
  setParameter(processor,"speedRange",2.f);
  setParameter(processor,"scratchAmount",100.f);
  setParameter(processor,"loopLock",1.f);
  processor.prepareToPlay(rate,256);
  REQUIRE(processor.getLatencySamples()==0);
  juce::AudioBuffer<float> block(2,256);
  juce::MidiBuffer midi;
  unsigned int rng=0xbadbeef0u;
  float maxOutput=0.f;
  const auto began=std::chrono::steady_clock::now();
  int readyAt=-1;
  for(int iteration=0;iteration<930;++iteration) {
   // First 430 blocks must be uninterrupted, so the loop captures even
   // at 192 kHz; the rest aggressively change parameters every few blocks.
   if(iteration>=430) {
    if(iteration%11==0) setParameter(processor,"scratchAmount",static_cast<float>((iteration*17)%101));
    if(iteration%13==0) setParameter(processor,"stopTrigger",iteration%26==0?1.f:0.f);
    if(iteration%17==0) setParameter(processor,"speedRange",static_cast<float>((iteration/17)%3));
    if(iteration%79==0) setParameter(processor,"loopLock",iteration%158==0?1.f:0.f);
    if(iteration%101==0) setParameter(processor,"loopLength",static_cast<float>((iteration/101)%5));
    if(iteration%109==0) {
     std::vector<CurveNode> nodes;
     nodes.reserve(40);
     for(int i=0;i<40;++i) {
      rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
      nodes.push_back({i/39.f,(float)((int)(rng%4001)-2000)/1000.f,0.f});
     }
     processor.setCurve(std::move(nodes));
     processor.collectRetiredCurves();
    }
   }
   for(int i=0;i<256;++i)for(int ch=0;ch<2;++ch) {
    rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
    const float noise=.25f*((rng&65535u)/32768.f-1.f);
    const float transient=((iteration*256+i)%3072==0)?(ch?-.93f:.93f):0.f;
    float v=noise+transient;
    if((iteration*256+i)%16001==0)v=std::numeric_limits<float>::quiet_NaN();
    if((iteration*256+i)%32771==0)v=std::numeric_limits<float>::infinity();
    if((iteration*256+i)%19311==0)v=1.e-38f;
    block.setSample(ch,i,v);
   }
   processor.processBlock(block,midi);
   if(processor.lockedStatus.load(std::memory_order_relaxed)==2 && readyAt<0)readyAt=iteration;
   for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i) {
    const float y=block.getSample(ch,i);
    REQUIRE(std::isfinite(y));
    REQUIRE(std::abs(y)<8.f); // floating-headroom bound, not hard clipping
    maxOutput=std::max(maxOutput,std::abs(y));
   }
  }
  REQUIRE(readyAt>=0);
  REQUIRE(maxOutput>.05f);
  REQUIRE(processor.getLatencySamples()==0);
  const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();
  const double duration=930.0*256.0/rate;
  std::printf("REAL PROCESSOR STRESS rate=%g Hz: %d blocks, lock ready @%d, peak=%.3f, DSP real-time load=%.1f%%\n",
              rate,930,readyAt,maxOutput,100.0*elapsed/duration);
  processor.releaseResources();
 }
}

void testActualEditor() {
    ScratchPadAudioProcessor processor;
    processor.prepareToPlay(sampleRate, frames);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    REQUIRE(editor != nullptr);
    const juce::File previews = juce::File::getCurrentWorkingDirectory()
        .getChildFile("ui-previews");
    REQUIRE(previews.createDirectory().wasOk());
    for (const auto size : {juce::Point<int>{441,263}, juce::Point<int>{882,526}, juce::Point<int>{1323,789}}) {
        editor->setSize(size.x, size.y);
        REQUIRE(editor->getWidth() == size.x && editor->getHeight() == size.y);
        const auto image = editor->createComponentSnapshot(editor->getLocalBounds());
        REQUIRE(image.isValid());
        REQUIRE(image.getWidth() == size.x && image.getHeight() == size.y);
        // The actual editor must paint Skelly-green accents, not a blank
        // screenshot. Inspect a coarse sample of pixels in the real render.
        int neonPixels = 0;
        for (int y = 0; y < size.y; y += 5)
            for (int x = 0; x < size.x; x += 5) {
                const auto c = image.getPixelAt(x, y);
                if (c.getGreen() >= 90 && c.getGreen() > c.getRed() * 1.2f &&
                    c.getGreen() > c.getBlue() * 1.1f) ++neonPixels;
            }
        REQUIRE(neonPixels > 10);
        // Verify the *actual painted* footer has an ivory skull + SKELL and
        // lime-green PAD word, following the Skell Key/Spacer house theme.
        auto designRect = [&](int x, int y, int w, int h) {
            return juce::Rectangle<int>(
                juce::roundToInt(x * (float)size.x / 441.f),
                juce::roundToInt(y * (float)size.y / 263.f),
                juce::roundToInt(w * (float)size.x / 441.f),
                juce::roundToInt(h * (float)size.y / 263.f));
        };
        auto pixels = [&](juce::Rectangle<int> area, bool lime) {
            int count=0;
            for(int yy=area.getY(); yy<area.getBottom(); ++yy)
                for(int xx=area.getX(); xx<area.getRight(); ++xx) {
                    const auto c=image.getPixelAt(xx,yy);
                    if(lime ? (c.getGreen()>180 && c.getGreen()>c.getRed()*1.14f && c.getGreen()>c.getBlue()*1.35f)
                            : (c.getRed()>170 && c.getGreen()>155 && c.getBlue()>125 &&
                               std::abs((int)c.getRed()-(int)c.getGreen())<35)) ++count;
                }
            return count;
        };
        REQUIRE(pixels(designRect(18,220,29,31),false)>12); // skull
        REQUIRE(pixels(designRect(52,220,56,25),false)>12); // SKELL
        REQUIRE(pixels(designRect(95,220,64,25),true)>12); // PAD, tightly kerned
        juce::PNGImageFormat format;
        juce::MemoryOutputStream pngData;
        REQUIRE(format.writeImageToStream(image, pngData));
        const auto path = previews.getChildFile("scratchpad-" + juce::String(size.x) +
                                                 "x" + juce::String(size.y) + ".png");
        REQUIRE(path.replaceWithData(pngData.getData(), pngData.getDataSize()));
        std::printf("Real JUCE editor: %dx%d PNG rendered; neon pixels %d\n",
                    size.x, size.y, neonPixels);
    }
    // Render the actual bottom strip with a known 0.75 FS stereo signal.
    // The meter must react to real processBlock output, not a dummy timer.
    juce::AudioBuffer<float> audio(2,128);
    juce::MidiBuffer midi;
    for(int block=0;block<8;++block){
        for(int ch=0;ch<2;++ch)for(int i=0;i<128;++i)
            audio.setSample(ch,i,.75f);
        processor.processBlock(audio,midi);
    }
    REQUIRE(processor.outputPeakHold.load(std::memory_order_relaxed)>.7f);
    juce::Thread::sleep(55);
    juce::Timer::callPendingTimersSynchronously();
    editor->setSize(882,526);
    const auto active=editor->createComponentSnapshot(editor->getLocalBounds());
    REQUIRE(active.isValid());
    int lit=0;
    for(int y=409;y<429;++y)for(int x=28;x<854;++x){
        const auto c=active.getPixelAt(x,y);
        if(c.getGreen()>170 && c.getGreen()>c.getRed()*1.2f &&
           c.getGreen()>c.getBlue()*1.3f) ++lit;
    }
    REQUIRE(lit>60);
    juce::PNGImageFormat png;
    juce::MemoryOutputStream activeBytes;
    REQUIRE(png.writeImageToStream(active,activeBytes));
    REQUIRE(previews.getChildFile("scratchpad-882x526-meter-active.png")
        .replaceWithData(activeBytes.getData(),activeBytes.getDataSize()));
    std::printf("Real JUCE audio meter: %d lime pixels in activity strip\n",lit);
    editor.reset();
    processor.releaseResources();
}
}

int main() {
    juce::ScopedJuceInitialiser_GUI gui;
    testActualAudio();
    testActualProcessorStress();
    testActualEditor();
    return 0;
}
