/* Shared clock fade and screen saver helpers. */
#pragma once

#include <FastLED.h>

namespace DisplayFadesNS
{
    uint8_t GetPaletteCount(void);
    const char* const* GetPaletteNames(void);
    CRGBPalette16 GetPalette(uint8_t aIndex, const CRGB* apColors);

    uint8_t FadeAmount(uint32_t aElapsedMs, uint32_t aDurationMs);
    CRGB FadeColor(const CRGB& arFrom, const CRGB& arTo, uint8_t aAmount);
    void StepColorTwinkle(CRGB& arColor, bool& arFadeUp, uint8_t aSpeed);
}