#pragma once
#include <cmath>
#include <algorithm>

// Allocation-free ring-buffer geometry, usable without JUCE by regression tests.
namespace HeadSafety {
inline double wrap(double p, int size) noexcept {
    if (size <= 0 || !std::isfinite(p)) return 0.0;
    p = std::fmod(p, static_cast<double>(size));
    return p < 0.0 ? p + static_cast<double>(size) : p;
}
// Audio-thread hot path. A scratch step is bounded to +/-4 samples, so
// a single branch handles normal wrapping without expensive fmod().
inline double advance(double position, double step, int size) noexcept {
    if (size <= 0 || !std::isfinite(position) || !std::isfinite(step)) return 0.0;
    if (position < 0.0 || position >= size || std::abs(step) >= size)
        return wrap(position + step, size);
    position += step;
    if (position >= size) position -= size;
    else if (position < 0.0) position += size;
    return position;
}
inline double age(int writePosition, double readPosition, int size) noexcept {
    return wrap(static_cast<double>(writePosition) - readPosition, size);
}
inline bool approachingOverwrite(int writePosition, double readPosition,
                                 int size, int protectedSamples) noexcept {
    if (size < 8) return false;
    const double behind = age(writePosition, readPosition, size);
    const double guard = static_cast<double>(std::clamp(protectedSamples, 4, size / 4));
    // Both sides of the recording head are unsafe: the immediately trailing
    // samples can be overwritten, and the leading samples are not yet recorded.
    return behind < guard || behind > static_cast<double>(size) - guard;
}
inline double safePosition(int writePosition, int size, int loopLength,
                           int protectedSamples) noexcept {
    const int margin = std::clamp(protectedSamples * 4, 16, std::max(16, size / 2));
    const int targetAge = std::clamp(loopLength, margin, std::max(margin, size - margin));
    return wrap(static_cast<double>(writePosition - targetAge), size);
}
}
