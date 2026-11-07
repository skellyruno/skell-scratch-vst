#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>

// Precomputed 32-tap, polyphase Blackman-windowed sinc interpolation.
// Selecting wider input filters before >1x playback reduces aliasing;
// coefficient generation happens only in prepareToPlay(), never on audio.
namespace ScratchDSP {
class BandlimitedInterpolator {
public:
    static constexpr int taps = 32;
    static constexpr int phases = 64;
    static constexpr int bands = 5;
    static constexpr std::array<float, bands> speeds {{1.f, 1.5f, 2.f, 3.f, 4.f}};

    void prepare() noexcept {
        constexpr double pi = 3.1415926535897932384626433832795;
        for (int band = 0; band < bands; ++band) {
            const double cutoff = 0.46 / static_cast<double>(speeds[band]);
            for (int phase = 0; phase <= phases; ++phase) {
                const double fraction = static_cast<double>(phase) / phases;
                double sum = 0.0;
                for (int tap = 0; tap < taps; ++tap) {
                    const double distance = fraction - static_cast<double>(tap - taps / 2 + 1);
                    const double normalized = distance / (taps / 2.0);
                    const double window = std::abs(normalized) >= 1.0 ? 0.0
                        : 0.42 + 0.5 * std::cos(pi * normalized)
                            + 0.08 * std::cos(2.0 * pi * normalized);
                    const double arg = 2.0 * cutoff * distance;
                    const double sinc = std::abs(arg) < 1.e-12 ? 1.0
                        : std::sin(pi * arg) / (pi * arg);
                    const double weight = 2.0 * cutoff * sinc * window;
                    weights[band][phase][tap] = static_cast<float>(weight);
                    sum += weight;
                }
                if (std::abs(sum) > 1.e-12)
                    for (int tap = 0; tap < taps; ++tap)
                        weights[band][phase][tap] = static_cast<float>(weights[band][phase][tap] / sum);
            }
        }
    }

    // Reader callback receives an in-range circular sample index. At high
    // speeds, coefficients interpolate between neighboring cutoff bands and
    // fractional phases, avoiding filter jumps during speed automation.
    template <typename Reader>
    float sample(double position, int length, float absSpeed, Reader&& read) const noexcept {
        if (length <= 0 || !std::isfinite(position)) return 0.f;
        if (!std::isfinite(absSpeed)) absSpeed = 1.f;
        const float rate = std::clamp(absSpeed, 1.f, 4.f);
        int band = 0;
        while (band + 2 < bands && rate > speeds[band + 1]) ++band;
        const float bandMix = (rate - speeds[band]) / (speeds[band + 1] - speeds[band]);
        const double integer = std::floor(position);
        const float fractional = static_cast<float>(position - integer);
        const float p = fractional * static_cast<float>(phases);
        const int phase = std::clamp(static_cast<int>(p), 0, phases - 1);
        const float phaseMix = p - phase;
        int index = static_cast<int>(integer) - taps / 2 + 1;
        double output = 0.0;
        for (int tap = 0; tap < taps; ++tap, ++index) {
            // Normal playback has wrapped positions and long buffers, so
            // this normally requires only a branch, not integer modulo.
            int wrapped = index;
            if (wrapped < 0) wrapped %= length, wrapped += length;
            else if (wrapped >= length) wrapped %= length;
            const float a = weights[band][phase][tap];
            const float b = weights[band][phase + 1][tap];
            const float c = weights[band + 1][phase][tap];
            const float d = weights[band + 1][phase + 1][tap];
            const float low = a + (b - a) * phaseMix;
            const float high = c + (d - c) * phaseMix;
            output += static_cast<double>(read(wrapped)) * (low + (high - low) * bandMix);
        }
        return std::isfinite(output) ? static_cast<float>(output) : 0.f;
    }
private:
    std::array<std::array<std::array<float, taps>, phases + 1>, bands> weights{};
};
} // namespace ScratchDSP
