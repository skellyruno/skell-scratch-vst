#include "../Source/BandlimitedInterpolation.h"
#include "../Source/LoopSeam.h"
#include "../Source/DSPHelpers.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

// Standalone performance profiler for the highest-cost DSP path. This is NOT
// a substitute for measuring the actual plugin on a DAW audio thread.
int main() {
    constexpr int length = 65536;
    std::vector<float> left(length),right(length);
    for (int i=0;i<length;++i) {
        left[static_cast<size_t>(i)] = static_cast<float>(.5 * std::sin(i*.021));
        right[static_cast<size_t>(i)] = static_cast<float>(.5 * std::sin(i*.019));
    }
    ScratchDSP::BandlimitedInterpolator interpolator;
    interpolator.prepare();
    ScratchDSP::LoopSeam seam;
    seam.prepare(length,192000.);
    seam.setBoundary(0,left.front(),left.back());
    seam.setBoundary(1,right.front(),right.back());
    for (int rate : {44100,48000,96000,192000}) {
        constexpr int seconds=2;
        volatile double result=0;
        const auto start=std::chrono::steady_clock::now();
        double position=14.3;
        for(int i=0;i<rate*seconds;++i) {
            result += interpolator.sample(position,length,4.f,[&](int j){
                return seam.sample(left[static_cast<size_t>(j)],j,0);
            });
            result += interpolator.sample(position,length,4.f,[&](int j){
                return seam.sample(right[static_cast<size_t>(j)],j,1);
            });
            position+=4.0;
            if(position>=length)position-=length;
        }
        const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        std::printf("rate=%dHz stereo x4 2s samples=%d elapsed=%.4fs processing/real-time=%.2f%% checksum=%.4f\n",
                    rate,rate*seconds,elapsed,100.*elapsed/seconds,static_cast<double>(result));
    }
}
