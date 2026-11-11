#include "TestCheck.h"
#include "../Source/SignalMeter.h"
#include "../Source/BandlimitedInterpolation.h"
#include "../Source/CurveMath.h"
#include "../Source/DSPHelpers.h"
#include "../Source/HeadSafety.h"
#include "../Source/LoopSeam.h"
#include "../Source/LockedLoop.h"
#include <algorithm>
#include <atomic>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <thread>
#include <vector>

static uint32_t rngState=0x9f12cd83u;
static uint32_t nextRandom(){rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5;return rngState;}
static float unit(){return static_cast<float>(nextRandom())/4294967296.0f;}

int main() {
    // Lock-free peak measurement: 4 publishers and one consumer are allowed
    // to race; a later isolated full-level event must always be observed.
    std::atomic<float> peak{0.f};
    std::array<std::thread,4> writers;
    for(int i=0;i<4;++i) writers[static_cast<size_t>(i)] = std::thread([&peak,i] {
        for(int n=0;n<250000;++n)
            ScratchDSP::publishPeak(peak, .1f + static_cast<float>(i)*.2f);
    });
    for(auto& worker:writers)worker.join();
    REQUIRE(std::abs(ScratchDSP::consumePeak(peak)-.7f)<.00001f);
    REQUIRE(ScratchDSP::consumePeak(peak)==0.f);
    ScratchDSP::publishPeak(peak,std::numeric_limits<float>::quiet_NaN());
    ScratchDSP::publishPeak(peak,-1.f);
    REQUIRE(ScratchDSP::consumePeak(peak)==0.f);
    ScratchDSP::publishPeak(peak,2.f);  // float headroom is not hard clipped
    REQUIRE(ScratchDSP::consumePeak(peak)==2.f);
    REQUIRE(ScratchDSP::peakToDisplay(0.f)==0.f);
    REQUIRE(ScratchDSP::peakToDisplay(std::numeric_limits<float>::quiet_NaN())==0.f);
    REQUIRE(ScratchDSP::peakToDisplay(1.f)==1.f);
    REQUIRE(ScratchDSP::peakToDisplay(1000.f)==1.f);
    REQUIRE(std::abs(ScratchDSP::peakToDisplay(.01f)-8.f/48.f)<.0001f);
    float displayed=0.f;
    for(int i=0;i<30;++i) displayed=ScratchDSP::meterFollow(displayed,i?0.f:1.f);
    REQUIRE(displayed>.001f && displayed<.02f);

    ScratchDSP::BandlimitedInterpolator sinc;
    sinc.prepare();
    constexpr int size=16384;
    std::vector<float> samples(size);
    for(int i=0;i<size;++i) samples[static_cast<size_t>(i)]=
        .75f * std::sin(i*.067f) + .15f*std::sin(i*.99f);
    ScratchDSP::LoopSeam seam;
    seam.prepare(size,192000.);
    seam.setBoundary(0,samples.front(),samples.back());
    double position=16383.9321;
    float peakOutput=0.f;
    auto began=std::chrono::steady_clock::now();
    for(int n=0;n<1000000;++n) {
        const float speed=(nextRandom()&1u?-1.f:1.f)*(.2f+4.0f*unit());
        const double step=static_cast<double>(speed);
        position=HeadSafety::advance(position,step,size);
        const float result=sinc.sample(position,size,std::abs(speed),[&](int index) {
            return seam.sample(samples[static_cast<size_t>(index)],index,0);
        });
        REQUIRE(std::isfinite(result));
        REQUIRE(std::abs(result)<1.1f);
        peakOutput=std::max(peakOutput,std::abs(result));
        if(n%8191==0){
            // A real host can seek or restart at arbitrary timelines.
            const double jump=static_cast<double>(static_cast<int>(nextRandom()%100000)-50000);
            position=HeadSafety::wrap(position+jump,size);
        }
    }
    REQUIRE(peakOutput>.01f);
    // Large virtual positions must stay finite without integer overflows.
    for(double p : {1.e8+17.25,1.e10+7.5,-1.e9+.25,-9.e8-17.75}) {
        for(float speed : {1.f,1.49f,2.f,3.f,4.f}) {
            const float v=sinc.sample(p,size,speed,[&](int index) {return samples[static_cast<size_t>(index)];});
            REQUIRE(std::isfinite(v) && std::abs(v)<=1.1f);
        }
    }

    // 2000 random editor curves, including thousands of duplicated and
    // out-of-range points, processed at cyclic transport-wrap positions.
    for(int caseIndex=0;caseIndex<2000;++caseIndex){
        std::vector<CurveNode> nodes;
        const int length=static_cast<int>(nextRandom()%800);
        nodes.reserve(static_cast<size_t>(length));
        for(int i=0;i<length;++i){
            float x=unit()*1.2f-.1f;
            if(i%27==0)x=.5f;
            float y=unit()*6.f-3.f;
            if(i%53==0)y=std::numeric_limits<float>::quiet_NaN();
            nodes.push_back({x,y,unit()*6.f-3.f});
        }
        CurveMath::normalize(nodes);
        REQUIRE(nodes.size()>=2 && nodes.size()<=512);
        REQUIRE(nodes.front().x==0.f && nodes.back().x==1.f);
        for(size_t i=1;i<nodes.size();++i)REQUIRE(nodes[i].x>=nodes[i-1].x);
        CurveMath::MonotonicCursor cursor(&nodes);
        for(int i=0;i<60;++i){
            const float x=(i%30)/29.f;
            const float v=cursor.value(x);
            REQUIRE(std::isfinite(v)&&std::abs(v)<=2.000001f);
            REQUIRE(std::abs(v-CurveMath::evaluate(nodes,x))<.00001f);
        }
    }
    // Tiny subnormal-range samples and NaN taps cannot escape as invalids.
    REQUIRE(sinc.sample(32.5,size,4.f,[](int){return 1e-38f;})>=0.f);
    REQUIRE(sinc.sample(32.5,size,4.f,[](int){return std::numeric_limits<float>::quiet_NaN();})==0.f);
    ScratchDSP::SpeedSlew speedSlew;
    for (double sampleRate : {44100.,48000.,96000.,192000.}) {
        speedSlew.prepare(sampleRate);speedSlew.reset();
        float last=0.f;
        for(int i=0;i<100000;++i){
            const float target=(i/1200%2==0?4.f:-4.f);
            const float value=speedSlew.process(target);
            REQUIRE(std::isfinite(value)&&std::abs(value)<=4.000001f);
            REQUIRE(std::abs(value-last)<1.f);
            last=value;
        }
    }
    const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();
    std::printf("STRESS: 1,000,000 sinc reads, 1,000,000 peak publications, 2000 random envelopes, \\n                400k speed changes: %.2fs (worst peak %.3f)\n",elapsed,peakOutput);
    return 0;
}
