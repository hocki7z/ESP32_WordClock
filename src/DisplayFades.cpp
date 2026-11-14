/*
 * Screen saver palette and Colortwinkles envelope adapted from WLED-MM,
 * https://github.com/MoonModules/WLED-MM, wled00/FX.cpp (EUPL-1.2).
 */
#include "DisplayFades.h"

#include <algorithm>

namespace DisplayFadesNS
{
    /* Preserve the indices of previously saved palette selections. */
    static const char* const mcPaletteNames[] = {
        "Default", "Rainbow", "Party", "Ocean", "Forest", "Lava", "Heat", "Cloud", "Rainbow Stripes",
        "Colors 1&2", "Colors 1-3"
    };

    uint8_t GetPaletteCount(void)
    {
        return sizeof(mcPaletteNames) / sizeof(mcPaletteNames[0]);
    }

    const char* const* GetPaletteNames(void)
    {
        return mcPaletteNames;
    }

    CRGBPalette16 GetPalette(uint8_t aIndex, const CRGB* apColors)
    {
        switch (aIndex)
        {
            case 1:  return RainbowColors_p;
            case 3:  return OceanColors_p;
            case 4:  return ForestColors_p;
            case 5:  return LavaColors_p;
            case 6:  return HeatColors_p;
            case 7:  return CloudColors_p;
            case 8:  return RainbowStripeColors_p;
            case 9:  return CRGBPalette16(apColors[0], apColors[1]);
            case 10: return CRGBPalette16(apColors[0], apColors[1], apColors[2]);
            default: return PartyColors_p;
        }
    }

    uint8_t ColorDistance(const CRGB& arFrom, const CRGB& arTo)
    {
        uint8_t wMax = 0;
        for (uint8_t wChannel = 0; wChannel < 3; wChannel++)
        {
            const int wDelta = int(arFrom[wChannel]) - int(arTo[wChannel]);
            const uint8_t wDistance = static_cast<uint8_t>((wDelta < 0) ? -wDelta : wDelta);
            if (wDistance > wMax) wMax = wDistance;
        }
        return wMax;
    }

    CRGB FadeColor(const CRGB& arFrom, const CRGB& arTo, uint16_t aAmount)
    {
        return CRGB(FadeChannel(arFrom.r, arTo.r, aAmount),
                    FadeChannel(arFrom.g, arTo.g, aAmount),
                    FadeChannel(arFrom.b, arTo.b, aAmount));
    }

    /** @brief Keeps the post-brightness output linear when global brightness changes during a fade. */
    CRGB FadeColorWithBrightness(const CRGB& arFrom, const CRGB& arTo, uint16_t aAmount,
        uint8_t aBrightnessFrom, uint8_t aBrightnessTo, uint8_t aBrightnessNow)
    {
        if (aBrightnessNow == 0) return CRGB::Black;

        auto wChannel = [aAmount, aBrightnessFrom, aBrightnessTo, aBrightnessNow](uint8_t aFrom, uint8_t aTo) -> uint8_t
        {
            const uint32_t wFrom = uint32_t(aFrom) * aBrightnessFrom * (UINT16_MAX - aAmount);
            const uint32_t wTo = uint32_t(aTo) * aBrightnessTo * aAmount;
            const uint32_t wEffective = (wFrom + wTo + UINT16_MAX / 2U) / UINT16_MAX;
            return static_cast<uint8_t>(std::min<uint32_t>(255, (wEffective + aBrightnessNow / 2U) / aBrightnessNow));
        };

        return CRGB(wChannel(arFrom.r, arTo.r), wChannel(arFrom.g, arTo.g), wChannel(arFrom.b, arTo.b));
    }

    void StepColorTwinkle(CRGB& arColor, bool& arFadeUp, uint8_t aSpeed)
    {
        if (arFadeUp)
        {
            const CRGB wPrev = arColor;
            CRGB wInc = arColor;
            wInc.nscale8_video(8 + (aSpeed >> 2));
            arColor += wInc;
            if ((arColor.r == 255) || (arColor.g == 255) || (arColor.b == 255)) arFadeUp = false;
            if (arColor == wPrev) arColor += arColor;
        }
        else
        {
            arColor.nscale8(255 - (8 + (aSpeed >> 3)));
        }
    }
}