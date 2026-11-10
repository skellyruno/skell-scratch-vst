#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// Small, allocation-free conditioning at the wrap of an immutable captured
// loop. The first and last samples are pulled toward the same midpoint with
// a smooth, sample-rate-dependent taper. This prevents a hard discontinuity
// when a scratch head crosses the capture boundary in either direction.
// The source audio remains unchanged and is never modified after recording.
namespace ScratchDSP {
class LoopSeam {
public:
    void prepare(int loopLength, double sampleRate) noexcept {
        length = std::max(0, loopLength);
        // 1.5 ms window. Small loops are capped at 1/4 their length so the
        // two taper regions never overlap or affect most of the recording.
        const int desired = std::isfinite(sampleRate) && sampleRate > 0
            ? std::max(2, static_cast<int>(std::round(sampleRate * .0015))) : 2;
        fade = length >= 8 ? std::clamp(desired, 2, length / 4) : 0;
        midpoint.fill(0.f);
    }
    void setBoundary(int channel, float first, float last) noexcept {
        if (channel < 0 || channel >= static_cast<int>(midpoint.size())) return;
        if (!std::isfinite(first)) first = 0.f;
        if (!std::isfinite(last)) last = 0.f;
        midpoint[static_cast<std::size_t>(channel)] = first * .5f + last * .5f;
    }
    float sample(float original, int index, int channel) const noexcept {
        if (!std::isfinite(original)) return 0.f;
        if (fade < 2 || index < 0 || index >= length ||
            channel < 0 || channel >= static_cast<int>(midpoint.size())) return original;
        const int distance = std::min(index, length - 1 - index);
        if (distance >= fade) return original;
        const float t = static_cast<float>(distance) / static_cast<float>(fade - 1);
        const float ease = t * t * (3.f - 2.f * t);
        return midpoint[static_cast<std::size_t>(channel)] * (1.f - ease) + original * ease;
    }
    int fadeSamples() const noexcept { return fade; }
private:
    int fade = 0, length = 0;
    std::array<float, 2> midpoint{};
};
} // namespace ScratchDSP
