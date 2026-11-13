/* Fixed-point helpers for clock transitions. */
#pragma once

#include <stdint.h>

namespace DisplayFadesNS
{
    /* A full-scale amount is 65535; a zero-duration phase completes immediately. */
    inline uint16_t FadeAmount(uint32_t aElapsedMs, uint32_t aDurationMs)
    {
        if ((aDurationMs == 0) || (aElapsedMs >= aDurationMs)) return UINT16_MAX;
        return static_cast<uint16_t>((uint64_t(aElapsedMs) * UINT16_MAX) / aDurationMs);
    }

    /* Interpolate without signed underflow; quantize only when producing an LED channel. */
    inline uint8_t FadeChannel(uint8_t aFrom, uint8_t aTo, uint16_t aAmount)
    {
        const uint32_t wWeighted = uint32_t(aFrom) * (UINT16_MAX - aAmount) + uint32_t(aTo) * aAmount;
        return static_cast<uint8_t>((wWeighted + UINT16_MAX / 2U) / UINT16_MAX);
    }
}