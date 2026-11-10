#include "../Source/LoopSeam.h"
#include "../Source/BandlimitedInterpolation.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

int main() {
    for (double sampleRate : {44100., 48000., 96000., 192000.}) {
        for (int size : {8, 32, 256, 8192}) {
            ScratchDSP::LoopSeam seam;
            seam.prepare(size,sampleRate);
            assert(seam.fadeSamples()>=2);
            assert(seam.fadeSamples()<=size/4);
            const float first=-.85f, last=.75f;
            seam.setBoundary(0,first,last);
            const float atStart=seam.sample(first,0,0);
            const float atEnd=seam.sample(last,size-1,0);
            assert(std::abs(atStart-atEnd)<1.e-6f);
            assert(std::abs(atStart-(first+last)*.5f)<1.e-6f);
            for(int i=0;i<size;++i) {
                const float raw=(i==0?first:(i==size-1?last:.55f));
                const float v=seam.sample(raw,i,0);
                assert(std::isfinite(v));
                assert(v>=-.85f && v<=.75f);
                if(i>=seam.fadeSamples() && i<size-seam.fadeSamples())
                    assert(v==raw);
            }
            // No changes to constant or already-periodic captures.
            seam.setBoundary(1,.2f,.2f);
            for(int i=0;i<size;++i) assert(std::abs(seam.sample(.2f,i,1)-.2f)<1.e-7f);
        }
    }
    ScratchDSP::LoopSeam seam;
    seam.prepare(0,48000.);
    assert(seam.fadeSamples()==0);
    assert(seam.sample(.75f,0,0)==.75f);
    assert(seam.sample(std::numeric_limits<float>::quiet_NaN(),0,0)==0.f);

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
            assert(std::isfinite(result));
            assert(std::abs(result)<=.900001f);
            position=std::fmod(position+speed+count,static_cast<double>(count));
        }
    }
    return 0;
}
