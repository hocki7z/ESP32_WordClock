/* Shared clock fade and screen saver helpers. */
#pragma once

#include <FastLED.h>
#include "DisplayFadeMath.h"

namespace DisplayFadesNS
{
    uint8_t GetPaletteCount(void);
    const char* const* GetPaletteNames(void);
    CRGBPalette16 GetPalette(uint8_t aIndex, const CRGB* apColors);

    /** @brief Largest absolute channel difference between two rendered colors. */
    uint8_t ColorDistance(const CRGB& arFrom, const CRGB& arTo);
    CRGB FadeColor(const CRGB& arFrom, const CRGB& arTo, uint16_t aAmount);
    CRGB FadeColorWithBrightness(const CRGB& arFrom, const CRGB& arTo, uint16_t aAmount,
        uint8_t aBrightnessFrom, uint8_t aBrightnessTo, uint8_t aBrightnessNow);
    void StepColorTwinkle(CRGB& arColor, bool& arFadeUp, uint8_t aSpeed);
}