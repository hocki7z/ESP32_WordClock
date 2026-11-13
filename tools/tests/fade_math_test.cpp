#include <cassert>
#include <cstdint>

#ifdef NDEBUG
#error Fade math tests require assertions to be enabled
#endif

#include "../../src/DisplayFadeMath.h"

int main()
{
    using namespace DisplayFadesNS;

    assert(FadeAmount(0, 0) == UINT16_MAX);
    assert(FadeAmount(0, 1000) == 0);
    assert(FadeAmount(500, 1000) == 32767);
    assert(FadeAmount(1000, 1000) == UINT16_MAX);
    assert(FadeAmount(UINT32_MAX, UINT32_MAX) == UINT16_MAX);
    assert(FadeAmount(UINT32_MAX - 1, UINT32_MAX) == 65534);

    for (unsigned wFrom = 0; wFrom <= UINT8_MAX; wFrom++)
    {
        for (unsigned wTo = 0; wTo <= UINT8_MAX; wTo++)
        {
            assert(FadeChannel(wFrom, wTo, 0) == wFrom);
            assert(FadeChannel(wFrom, wTo, UINT16_MAX) == wTo);
        }
        assert(FadeChannel(wFrom, wFrom, 32767) == wFrom);
    }

    assert(FadeChannel(0, 255, 32768) == 128);
    assert(FadeChannel(255, 0, 32768) == 127);
    assert(FadeChannel(0, 1, 32768) == 1);
    assert(FadeChannel(1, 0, 32768) == 0);

    uint8_t wUp = 0;
    uint8_t wDown = 255;
    for (uint32_t wAmount = 0; wAmount <= UINT16_MAX; wAmount++)
    {
        const uint8_t wNextUp = FadeChannel(0, 255, wAmount);
        const uint8_t wNextDown = FadeChannel(255, 0, wAmount);
        assert(wNextUp >= wUp);
        assert(wNextDown <= wDown);
        wUp = wNextUp;
        wDown = wNextDown;
    }
}