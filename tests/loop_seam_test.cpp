#include "TestCheck.h"
#include "../Source/LoopSeam.h"
#include "../Source/BandlimitedInterpolation.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

int main() {
    for (double sampleRate : {44100., 48000., 96000., 192000.}) {
        for (int size : {8, 32, 256, 8192}) {
            ScratchDSP::LoopSeam seam;
            seam.prepare(size,sampleRate);
            REQUIRE(seam.fadeSamples()>=2);
            REQUIRE(seam.fadeSamples()<=size/4);
            const float first=-.85f, last=.75f;
            seam.setBoundary(0,first,last);
            const float atStart=seam.sample(first,0,0);
            const float atEnd=seam.sample(last,size-1,0);
            REQUIRE(std::abs(atStart-atEnd)<1.e-6f);
            REQUIRE(std::abs(atStart-(first+last)*.5f)<1.e-6f);
            for(int i=0;i<size;++i) {
                const float raw=(i==0?first:(i==size-1?last:.55f));
                const float v=seam.sample(raw,i,0);
                REQUIRE(std::isfinite(v));
                REQUIRE(v>=-.85f && v<=.75f);
                if(i>=seam.fadeSamples() && i<size-seam.fadeSamples())
                    REQUIRE(v==raw);
            }
            // No changes to constant or already-periodic captures.
            seam.setBoundary(1,.2f,.2f);
            for(int i=0;i<size;++i) REQUIRE(std::abs(seam.sample(.2f,i,1)-.2f)<1.e-7f);
        }
    }
    ScratchDSP::LoopSeam seam;
    seam.prepare(0,48000.);
    REQUIRE(seam.fadeSamples()==0);
    REQUIRE(seam.sample(.75f,0,0)==.75f);
    REQUIRE(seam.sample(std::numeric_limits<float>::quiet_NaN(),0,0)==0.f);

    // Stress a deliberately discontinuous periodic loop at reverse and 4x,
    // including fractional positions, verifying no invalid/above-input peaks.
    constexpr int count=2048;
    std::vector<float> samples(count);
    for(int i=0;i<count;++i) samples[i]=.65f * std::sin(i*.023f);
    samples.front()=-.9f;samples.back()=.9f;
    seam.prepare(count,48000.);
    seam.setBoundary(0,samples.front(),samples.back());
    ScratchDSP::BandlimitedInterpolator interpolator;
    interpolator.prepare();
    for(float speed : {-4.f,-2.f,2.f,4.f}) {
        double position=count-3.25;
        for(int i=0;i<12000;++i) {
            const float result=interpolator.sample(position,count,std::abs(speed),[&](int index){
                return seam.sample(samples[static_cast<std::size_t>(index)],index,0);
            });
            REQUIRE(std::isfinite(result));
            REQUIRE(std::abs(result)<=.900001f);
            position=std::fmod(position+speed+count,static_cast<double>(count));
        }
    }
    // Compare the maximum adjacent-sample jump at the captured-loop seam
    // with and without conditioning, for both directions and 2x/4x. The
    // source is deliberately discontinuous; the assertion verifies that
    // the new seam treatment actually reduces an audible click precursor.
    std::vector<float> abrupt(count);
    for(int i=0;i<count;++i) abrupt[i]=.05f*std::sin(i*.02f);
    abrupt.front()=-.9f;
    abrupt.back()=.9f;
    seam.prepare(count,48000.);
    seam.setBoundary(0,abrupt.front(),abrupt.back());
    for (float rate : {-4.f,-2.f,2.f,4.f}) {
        const double start=rate>0.f?count-128.25:127.25;
        float previousRaw=0.f,previousSmooth=0.f;
        float maxRawDelta=0.f,maxSmoothDelta=0.f;
        for(int i=0;i<80;++i) {
            const double position=std::fmod(start+i*rate+count*4.,static_cast<double>(count));
            const float raw=interpolator.sample(position,count,std::abs(rate),[&](int index){
                return abrupt[static_cast<std::size_t>(index)];
            });
            const float smooth=interpolator.sample(position,count,std::abs(rate),[&](int index){
                return seam.sample(abrupt[static_cast<std::size_t>(index)],index,0);
            });
            if(i>0){
                maxRawDelta=std::max(maxRawDelta,std::abs(raw-previousRaw));
                maxSmoothDelta=std::max(maxSmoothDelta,std::abs(smooth-previousSmooth));
            }
            previousRaw=raw;previousSmooth=smooth;
        }
        REQUIRE(maxRawDelta>.04f);
        REQUIRE(maxSmoothDelta<maxRawDelta*.15f);
    }
    return 0;
}
