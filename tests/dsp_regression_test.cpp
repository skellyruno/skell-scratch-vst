#include "../Source/DSPHelpers.h"
#include "../Source/HeadSafety.h"
#include "../Source/CurveMath.h"
#include "../Source/ui/PresetBank.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

static bool near(double a, double b, double tol = 1.e-5) { return std::abs(a-b)<tol; }
int main() {
    // Exact dry bypass, exactly wet at 100%, and bounded equal-gain blend.
    assert(near(ScratchDSP::mixDryWet(.75f,-.1f,0.f),.75f));
    assert(near(ScratchDSP::mixDryWet(.75f,-.1f,1.f),-.1f));
    assert(near(ScratchDSP::mixDryWet(.75f,-.1f,.5f),.325f));
    assert(near(ScratchDSP::mixDryWet(1.f,1.f,.5f),1.f));
    assert(near(ScratchDSP::mixDryWet(1.f,2.f,-10.f),1.f));
    assert(near(ScratchDSP::transition(.75f,.75f,.5f),.75f));
    assert(near(ScratchDSP::transition(0.f,1.f,0.f),0.f));
    assert(near(ScratchDSP::transition(0.f,1.f,1.f),1.f));
    assert(near(ScratchDSP::smoothstep(.3f),1.f-ScratchDSP::smoothstep(.7f)));

    // Startup warms in from true dry to wet, rather than replaying zeros.
    assert(near(ScratchDSP::captureFade(0,96),0.f));
    assert(near(ScratchDSP::captureFade(4,96),0.f));
    assert(near(ScratchDSP::captureFade(52,96),.5f));
    assert(near(ScratchDSP::captureFade(100,96),1.f));
    for(float t : {0.f,.25f,.5f,.75f,1.f}) {
        assert(near(ScratchDSP::hermiteBounded(1.f,1.f,1.f,1.f,t),1.f));
        const float imp=ScratchDSP::hermiteBounded(0.f,0.f,1.f,0.f,t);
        assert(imp>=0.f && imp<=1.f);
        assert(std::isfinite(imp));
    }
    assert(near(ScratchDSP::hermiteBounded(0.f,0.f,0.f,1.f,0.f),0.f));
    assert(ScratchDSP::hermiteBounded(0.f,NAN,1.f,0.f,.5f)==0.f);

    // Fast bounded forward/reverse wrap is equivalent to general modulo wrap.
    for (int size : {8, 64, 4096})
        for (double start : {0.0, .25, 1.5, 5.75, 7.999})
            for (double step : {-4.,-1.,-.125,0.,.125,1.,4.}) {
                const double p = HeadSafety::wrap(start,size);
                assert(near(HeadSafety::advance(p,step,size),HeadSafety::wrap(p+step,size)));
            }
    assert(HeadSafety::advance(0, std::numeric_limits<double>::quiet_NaN(), 4096)==0.0);

    // Fast per-block cursor must agree with reference binary-search evaluation
    // during forward motion, at node boundaries, and after wrap.
    for(int id=0; id<ScratchPresets::count(); ++id) {
        auto curve=ScratchPresets::make(id);CurveMath::normalize(curve);
        CurveMath::MonotonicCursor cursor(&curve);
        for(int pass=0; pass<3; ++pass)
            for(int i=0; i<=8192; ++i) {
                const float x=static_cast<float>(i)/8192.f;
                assert(near(cursor.value(x),CurveMath::evaluate(curve,x),1.e-4));
            }
    }
    CurveMath::MonotonicCursor empty(nullptr);
    assert(empty.value(0.5f)==0.f);
    return 0;
}
