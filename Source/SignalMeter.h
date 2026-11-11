#pragma once
#include <atomic>
#include <algorithm>
#include <cmath>

// Thread-safe peak hold. The DSP publishes the maximum processed output
// amplitude once per block; the editor consumes once per frame, so transients
// between UI refreshes are never lost. No locks or allocations in audio.
namespace ScratchDSP {
inline void publishPeak(std::atomic<float>& peak, float value) noexcept {
    if (!std::isfinite(value) || value <= 0.f) return;
    auto seen = peak.load(std::memory_order_relaxed);
    while (seen < value && !peak.compare_exchange_weak(
        seen, value, std::memory_order_relaxed, std::memory_order_relaxed)) {}
}
inline float consumePeak(std::atomic<float>& peak) noexcept {
    return peak.exchange(0.f, std::memory_order_relaxed);
}
// Same -48..0 dB visual range as Skell Key. Values > 0 dBFS saturate the
// meter display only, never the actual floating-point output signal.
inline float peakToDisplay(float value) noexcept {
    if (!std::isfinite(value) || value <= 0.f) return 0.f;
    const float db = 20.f * std::log10(std::max(value, 1.e-12f));
    return std::clamp((db + 48.f)/48.f, 0.f, 1.f);
}
inline float meterFollow(float current, float target) noexcept {
    if (!std::isfinite(current)) current = 0.f;
    if (!std::isfinite(target)) target = 0.f;
    return std::clamp(std::max(target, current * .84f), 0.f, 1.f);
}
} // namespace ScratchDSP
