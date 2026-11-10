#include "TestCheck.h"
#include "../Source/HeadSafety.h"
#include <cmath>
int main() {
    // The capture gate must hold until a complete requested loop exists.
    REQUIRE(!HeadSafety::hasFullCapture(0, 24000));
    REQUIRE(!HeadSafety::hasFullCapture(24007, 24000));
    REQUIRE(HeadSafety::hasFullCapture(24008, 24000));
    REQUIRE(!HeadSafety::hasFullCapture(8000, 24000));
    for (int capacity : {128, 48000}) {
        const int loop=capacity-8;
        for (int write : {0, 5, capacity-1}) {
            const auto read=HeadSafety::capturedLoopStart(write,loop,capacity);
            const auto age=HeadSafety::age(write,read,capacity);
            REQUIRE(age>=0.0 && age<loop);
            REQUIRE(age<=capacity-16);
        }
    }

    constexpr int n=4096;
    REQUIRE(HeadSafety::wrap(-1.0,n)==4095.0);
    REQUIRE(HeadSafety::wrap(4097.5,n)==1.5);
    REQUIRE(HeadSafety::age(100,50.0,n)==50.0);
    REQUIRE(HeadSafety::approachingOverwrite(100,99.0,n,40));
    REQUIRE(HeadSafety::approachingOverwrite(100,101.0,n,40));
    REQUIRE(!HeadSafety::approachingOverwrite(100,500.0,n,40));
    REQUIRE(!HeadSafety::approachingOverwrite(100,1000.0,n,40));
    const auto p=HeadSafety::safePosition(10,n,800,40);
    REQUIRE(!HeadSafety::approachingOverwrite(10,p,n,40));
    REQUIRE(std::isfinite(HeadSafety::wrap(1.e12,n)));
    REQUIRE(HeadSafety::wrap(NAN,n)==0.0);
    return 0;
}
