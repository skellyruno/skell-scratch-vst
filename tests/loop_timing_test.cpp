#include "TestCheck.h"
#include "../Source/LoopTiming.h"
#include <cmath>
#include <limits>
int main() {
    REQUIRE(ScratchTiming::beatsForLengthChoice(-2)==1);
    REQUIRE(ScratchTiming::beatsForLengthChoice(2)==4);
    REQUIRE(ScratchTiming::beatsForLengthChoice(10)==16);
    REQUIRE(std::abs(ScratchTiming::snapNormalized(.037f,0)-.0625f)<1.e-6f);
    REQUIRE(std::abs(ScratchTiming::snapNormalized(.037f,4)-.03515625f)<1.e-6f);
    REQUIRE(ScratchTiming::snapNormalized(-1.f,3)==0.f);
    REQUIRE(ScratchTiming::snapNormalized(2.f,3)==1.f);
    REQUIRE(ScratchTiming::snapNormalized(std::numeric_limits<float>::quiet_NaN(),2)==0.f);
    for (int choice=0;choice<5;++choice) {
        const double beats=ScratchTiming::beatsForLengthChoice(choice);
        REQUIRE(ScratchTiming::phaseFromPpq(0.,9600,choice)==0);
        REQUIRE(ScratchTiming::phaseFromPpq(beats,9600,choice)==0);
        REQUIRE(ScratchTiming::phaseFromPpq(beats*.5,9600,choice)==4800);
        REQUIRE(ScratchTiming::phaseFromPpq(-beats*.25,9600,choice)==7200);
        REQUIRE(ScratchTiming::phaseFromPpq(beats*100+.25*beats,9600,choice)==2400);
    }
    REQUIRE(ScratchTiming::phaseFromPpq(std::numeric_limits<double>::infinity(),9600,2)==0);
    REQUIRE(ScratchTiming::phaseFromPpq(2.,0,2)==0);
    return 0;
}
