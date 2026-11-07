#include "../Source/LoopTiming.h"
#include <cassert>
#include <cmath>
#include <limits>
int main() {
    assert(ScratchTiming::beatsForLengthChoice(-2)==1);
    assert(ScratchTiming::beatsForLengthChoice(2)==4);
    assert(ScratchTiming::beatsForLengthChoice(10)==16);
    assert(std::abs(ScratchTiming::snapNormalized(.037f,0)-.0625f)<1.e-6f);
    assert(std::abs(ScratchTiming::snapNormalized(.037f,4)-.03515625f)<1.e-6f);
    assert(ScratchTiming::snapNormalized(-1.f,3)==0.f);
    assert(ScratchTiming::snapNormalized(2.f,3)==1.f);
    assert(ScratchTiming::snapNormalized(std::numeric_limits<float>::quiet_NaN(),2)==0.f);
    for (int choice=0;choice<5;++choice) {
        const double beats=ScratchTiming::beatsForLengthChoice(choice);
        assert(ScratchTiming::phaseFromPpq(0.,9600,choice)==0);
        assert(ScratchTiming::phaseFromPpq(beats,9600,choice)==0);
        assert(ScratchTiming::phaseFromPpq(beats*.5,9600,choice)==4800);
        assert(ScratchTiming::phaseFromPpq(-beats*.25,9600,choice)==7200);
        assert(ScratchTiming::phaseFromPpq(beats*100+.25*beats,9600,choice)==2400);
    }
    assert(ScratchTiming::phaseFromPpq(std::numeric_limits<double>::infinity(),9600,2)==0);
    assert(ScratchTiming::phaseFromPpq(2.,0,2)==0);
    return 0;
}
