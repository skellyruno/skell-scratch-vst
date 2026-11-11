#include "../Source/PluginProcessor.h"
#include "TestCheck.h"
#include <JuceHeader.h>
#include <algorithm>
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
    setParameter(processor, "sync", 0.f);
    setParameter(processor, "loopLength", 0.f);
    setParameter(processor, "scratchAmount", 0.f);
    setParameter(processor, "stopTrigger", 0.f);
    setParameter(processor, "loopLock", 0.f);
    processor.prepareToPlay(sampleRate, frames);
    REQUIRE(processor.getLatencySamples() == 0);
    BlockRunner runner{processor};
    const auto tone = [](std::int64_t i, int ch) {
        return static_cast<float>(.23 * std::sin(2 * pi * (ch == 0 ? 220. : 330.) * i / sampleRate));
    };
    // Tests the real processBlock, not just the mix helper. After 1s of
    // history at 0% depth output must remain bit-exact dry.
    for (int block = 0; block < 375; ++block) runner.run(tone, true);

    // New capture starts from the very next input sample. This proves that
    // silence fed later cannot overwrite the independently locked loop.
    setParameter(processor, "scratchAmount", 100.f);
    setParameter(processor, "loopLock", 1.f);
    for (int block = 0; block < 70; ++block) runner.run(tone);
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
    processor.releaseResources();
    std::printf("Real JUCE DSP: dry passthrough, immutable capture, stop, release, NaN and latency passed\n");
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
        juce::PNGImageFormat format;
        juce::MemoryOutputStream pngData;
        REQUIRE(format.writeImageToStream(image, pngData));
        const auto path = previews.getChildFile("scratchpad-" + juce::String(size.x) +
                                                 "x" + juce::String(size.y) + ".png");
        REQUIRE(path.replaceWithData(pngData.getData(), pngData.getDataSize()));
        std::printf("Real JUCE editor: %dx%d PNG rendered; neon pixels %d\n",
                    size.x, size.y, neonPixels);
    }
    editor.reset();
    processor.releaseResources();
}
}

int main() {
    juce::ScopedJuceInitialiser_GUI gui;
    testActualAudio();
    testActualEditor();
    return 0;
}
