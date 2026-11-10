#include "TestCheck.h"
#include "../Source/BandlimitedInterpolation.h"
#include "../Source/LockedLoop.h"
#include <array>
#include <cmath>
#include <limits>
#include <vector>

int main() {
    // Captured sample storage is frozen even as external input changes.
    ScratchCapture::LockedLoop state;
    state.start(256, 1024);
    std::vector<float> locked(1024, -99.f);
    for (int i=0;i<256;++i) {
        REQUIRE(state.nextWriteIndex()==i);
        REQUIRE(!state.ready);
        locked[static_cast<size_t>(state.nextWriteIndex())]=static_cast<float>(i);
        const bool done=state.advance();
        REQUIRE(done==(i==255));
    }
    REQUIRE(state.ready && !state.recording);
    for (int i=0;i<20000;++i) {
        REQUIRE(state.nextWriteIndex()==-1);
        REQUIRE(locked[static_cast<size_t>(i%256)]==static_cast<float>(i%256));
    }
    state.cancel();
    REQUIRE(state.ready);
    state.start(5000,1024);
    REQUIRE(state.length==1024 && !state.ready && state.recording);
    REQUIRE(state.progress()==0.f);
    state.reset();
    REQUIRE(!state.ready && !state.recording);

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
    REQUIRE(amplitude(9000.,4.f)<.085);
    // 1 kHz original content remains audible after 4x playback.
    REQUIRE(amplitude(1000.,4.f)>.94);
    const std::array<float,6> rates{1.f,1.2f,1.5f,2.f,3.f,4.f};
    std::vector<float> fixed(n, .37f);
    for(float rate:rates) {
        for(double p : {0.,.125,1.99,512.3,8191.7}) {
            const float v=sinc.sample(p,n,rate,[&](int j){return fixed[static_cast<size_t>(j)];});
            REQUIRE(std::isfinite(v));
            REQUIRE(std::abs(v-.37f)<.00001f);
        }
    }
    REQUIRE(sinc.sample(std::numeric_limits<double>::quiet_NaN(),n,4.f,
        [&](int j){return fixed[static_cast<size_t>(j)];})==0.f);
    // Windowed sinc ringing must not overshoot the peak of its source taps.
    // Also verifies arbitrary float headroom is preserved, not hard-clipped.
    std::vector<float> impulse(n);
    impulse[100]=1.f;
    for (float speed : {1.f,1.5f,2.f,3.f,4.f})
        for(double pos=80.;pos<122.;pos+=.125) {
            const float v=sinc.sample(pos,n,speed,[&](int i){return impulse[static_cast<size_t>(i)];});
            REQUIRE(std::isfinite(v) && std::abs(v)<=1.000001f);
        }
    impulse[100]=2.5f;
    REQUIRE(sinc.sample(100.,n,4.f,[&](int i){return impulse[static_cast<size_t>(i)];})>0.01f);
    REQUIRE(sinc.sample(100.,n,4.f,[&](int i){return impulse[static_cast<size_t>(i)];})<=2.5f);
    REQUIRE(sinc.sample(100.,n,4.f,[&](int i){return i==100?NAN:0.f;})==0.f);
    const float periodic=sinc.sample(512.125,n,2.f,[&](int i){return fixed[static_cast<size_t>(i)];});
    for(double outOfRange : {-n*3. + 512.125, n*2. + 512.125, n*1000000. + 512.125})
        REQUIRE(std::abs(sinc.sample(outOfRange,n,2.f,[&](int i){return fixed[static_cast<size_t>(i)];})-periodic)<1.e-5f);
    return 0;
}
