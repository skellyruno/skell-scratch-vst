#include "../Source/BandlimitedInterpolation.h"
#include "../Source/LockedLoop.h"
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

int main() {
    // Captured sample storage is frozen even as external input changes.
    ScratchCapture::LockedLoop state;
    state.start(256, 1024);
    std::vector<float> locked(1024, -99.f);
    for (int i=0;i<256;++i) {
        assert(state.nextWriteIndex()==i);
        assert(!state.ready);
        locked[static_cast<size_t>(state.nextWriteIndex())]=static_cast<float>(i);
        const bool done=state.advance();
        assert(done==(i==255));
    }
    assert(state.ready && !state.recording);
    for (int i=0;i<20000;++i) {
        assert(state.nextWriteIndex()==-1);
        assert(locked[static_cast<size_t>(i%256)]==static_cast<float>(i%256));
    }
    state.cancel();
    assert(state.ready);
    state.start(5000,1024);
    assert(state.length==1024 && !state.ready && state.recording);
    assert(state.progress()==0.f);
    state.reset();
    assert(!state.ready && !state.recording);

    ScratchDSP::BandlimitedInterpolator sinc;
    sinc.prepare();
    constexpr int n=8192;
    constexpr double pi=3.14159265358979323846;
    auto amplitude = [&](double frequency, float speed) {
        std::vector<float> input(n);
        for(int i=0;i<n;++i) input[static_cast<size_t>(i)] =
            static_cast<float>(std::sin(2*pi*frequency*i/48000.));
        double squared=0., reference=0.;
        for(int i=0;i<n/4;++i) {
            const int index=i*4;
            const float raw=input[static_cast<size_t>(index)];
            const float filtered=sinc.sample(static_cast<double>(index),n,speed,
                [&](int p){return input[static_cast<size_t>(p)];});
            squared+=static_cast<double>(filtered)*filtered;
            reference+=static_cast<double>(raw)*raw;
        }
        return std::sqrt(squared/reference);
    };
    // 9 kHz at 4x is above the allowable 6 kHz input cutoff.
    // The aliasing component must be strongly suppressed before resampling.
    assert(amplitude(9000.,4.f)<.085);
    // 1 kHz original content remains audible after 4x playback.
    assert(amplitude(1000.,4.f)>.94);
    const std::array<float,6> rates{1.f,1.2f,1.5f,2.f,3.f,4.f};
    std::vector<float> fixed(n, .37f);
    for(float rate:rates) {
        for(double p : {0.,.125,1.99,512.3,8191.7}) {
            const float v=sinc.sample(p,n,rate,[&](int j){return fixed[static_cast<size_t>(j)];});
            assert(std::isfinite(v));
            assert(std::abs(v-.37f)<.00001f);
        }
    }
    assert(sinc.sample(std::numeric_limits<double>::quiet_NaN(),n,4.f,
        [&](int j){return fixed[static_cast<size_t>(j)];})==0.f);
    return 0;
}
