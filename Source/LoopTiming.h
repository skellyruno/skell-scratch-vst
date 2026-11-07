#pragma once
#include <algorithm>
#include <cmath>

// Host musical-time conversion and grid geometry shared by the editor and DSP.
// No JUCE dependency: these functions are exercised by standalone tests.
namespace ScratchTiming {
inline int beatsForLengthChoice(int index) noexcept {
    constexpr int beats[] {1, 2, 4, 8, 16};
    return beats[std::clamp(index, 0, 4)];
}
inline float snapNormalized(float x, int lengthChoice, int divisionsPerBeat = 16) noexcept {
    if (!std::isfinite(x)) return 0.f;
    const int steps = beatsForLengthChoice(lengthChoice) * std::clamp(divisionsPerBeat, 1, 64);
    return std::clamp(std::round(std::clamp(x, 0.f, 1.f) * steps) / steps, 0.f, 1.f);
}
// PPQ = quarter notes from the DAW's musical origin; a 1-bar curve is 4 beats.
// Only used to anchor a restart/seek, never to force a mid-block timeline jump.
inline int phaseFromPpq(double ppq, int loopSamples, int lengthChoice) noexcept {
    if (!std::isfinite(ppq) || loopSamples <= 1) return 0;
    const double beats = static_cast<double>(beatsForLengthChoice(lengthChoice));
    double wrapped = std::fmod(ppq, beats);
    if (wrapped < 0.0) wrapped += beats;
    const auto sample = static_cast<int>(std::floor(wrapped / beats * loopSamples));
    return std::clamp(sample, 0, loopSamples - 1);
}
} // namespace ScratchTiming
