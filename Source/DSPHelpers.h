#pragma once
#include <algorithm>
#include <cmath>

namespace ScratchDSP {
// Gentle, sample-rate-independent speed slewing prevents zipper clicks when
// a node moves or the host automates an abrupt curve change. It does not
// allocate or call into JUCE from the audio callback.
struct SpeedSlew {
    double coefficient = 1.0;
    float current = 0.f;
    void prepare(double sampleRate, double timeSeconds = 0.0004) noexcept {
        if (!std::isfinite(sampleRate) || sampleRate <= 0.0) { coefficient = 1.0; return; }
        const double samples = std::max(1.0, sampleRate * std::max(timeSeconds, 1.e-6));
        coefficient = 1.0 - std::exp(-1.0 / samples);
    }
    void reset(float value = 0.f) noexcept { current = std::isfinite(value) ? value : 0.f; }
    float process(float target) noexcept {
        if (!std::isfinite(target)) target = 0.f;
        current += static_cast<float>(coefficient * (static_cast<double>(target) - current));
        if (!std::isfinite(current)) current = 0.f;
        return current;
    }
};
// Equal-gain blends avoid boosting two correlated read heads. A linear
// blend also guarantees exact input passthrough at a zero scratch amount.
inline float mixDryWet(float dry, float wet, float amount) noexcept {
    amount = std::clamp(amount, 0.f, 1.f);
    return dry + (wet - dry) * amount;
}
inline float smoothstep(float progress) noexcept {
    progress = std::clamp(progress, 0.f, 1.f);
    return progress * progress * (3.f - 2.f * progress);
}
inline float transition(float oldSample, float newSample, float progress) noexcept {
    return mixDryWet(oldSample, newSample, smoothstep(progress));
}
// Never blend abruptly from unrecorded history into a processed signal.
inline float captureFade(int capturedSamples, int fadeSamples) noexcept {
    const float progress = static_cast<float>(capturedSamples - 4) /
                           static_cast<float>(std::max(1, fadeSamples));
    return smoothstep(progress);
}
// Cubic Hermite with min/max bounding prevents ringing spikes between
// harsh transients; also rejects non-finite input from damaged host buffers.
inline float hermiteBounded(float a, float b, float c, float d, float t) noexcept {
    if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c) || !std::isfinite(d))
        return 0.f;
    t = std::clamp(t, 0.f, 1.f);
    const float c0 = b;
    const float c1 = .5f * (c - a);
    const float c2 = a - 2.5f*b + 2.f*c - .5f*d;
    const float c3 = .5f*(d - a) + 1.5f*(b - c);
    const float value = ((c3*t+c2)*t+c1)*t+c0;
    if (!std::isfinite(value)) return 0.f;
    return std::clamp(value, std::min({a,b,c,d}), std::max({a,b,c,d}));
}
} // namespace ScratchDSP
