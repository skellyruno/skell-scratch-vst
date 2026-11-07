#pragma once
#include <algorithm>

// Allocation-free state machine. Sample storage is preallocated and owned
// separately by the JUCE processor. Nothing in advance() reallocates memory.
namespace ScratchCapture {
class LockedLoop {
public:
    void reset() noexcept { length = 0; write = 0; ready = recording = false; }
    void start(int samples, int capacity) noexcept {
        length = std::clamp(samples, 1, std::max(1, capacity));
        write = 0; ready = false; recording = true;
    }
    void cancel() noexcept { recording = false; }
    // Returns the destination index before writing the next input sample.
    int nextWriteIndex() const noexcept { return recording ? write : -1; }
    // Advance after every channel has recorded the current input sample.
    bool advance() noexcept {
        if (!recording) return false;
        if (++write >= length) {
            recording = false; ready = true;
            return true;
        }
        return false;
    }
    float progress() const noexcept {
        return ready ? 1.f : recording && length > 0 ? static_cast<float>(write) / length : 0.f;
    }
    int length = 0;
    bool recording = false;
    bool ready = false;
private:
    int write = 0;
};
} // namespace ScratchCapture
