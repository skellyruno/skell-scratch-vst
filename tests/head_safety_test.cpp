#include "../Source/HeadSafety.h"
#include <cassert>
#include <cmath>
int main() {
    // The capture gate must hold until a complete requested loop exists.
    assert(!HeadSafety::hasFullCapture(0, 24000));
    assert(!HeadSafety::hasFullCapture(24007, 24000));
    assert(HeadSafety::hasFullCapture(24008, 24000));
    assert(!HeadSafety::hasFullCapture(8000, 24000));
    for (int capacity : {128, 48000}) {
        const int loop=capacity-8;
        for (int write : {0, 5, capacity-1}) {
            const auto read=HeadSafety::capturedLoopStart(write,loop,capacity);
            const auto age=HeadSafety::age(write,read,capacity);
            assert(age>=0.0 && age<loop);
            assert(age<=capacity-16);
        }
    }

    constexpr int n=4096;
    assert(HeadSafety::wrap(-1.0,n)==4095.0);
    assert(HeadSafety::wrap(4097.5,n)==1.5);
    assert(HeadSafety::age(100,50.0,n)==50.0);
    assert(HeadSafety::approachingOverwrite(100,99.0,n,40));
    assert(HeadSafety::approachingOverwrite(100,101.0,n,40));
    assert(!HeadSafety::approachingOverwrite(100,500.0,n,40));
    assert(!HeadSafety::approachingOverwrite(100,1000.0,n,40));
    const auto p=HeadSafety::safePosition(10,n,800,40);
    assert(!HeadSafety::approachingOverwrite(10,p,n,40));
    assert(std::isfinite(HeadSafety::wrap(1.e12,n)));
    assert(HeadSafety::wrap(NAN,n)==0.0);
    return 0;
}
