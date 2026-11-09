/*
 * Effects.h
 *
 *  Created on: 09.11.2026
 *      Author: hocki
 *
 *  2D matrix effects ported from WLED-MM (https://github.com/MoonModules/WLED-MM, wled00/FX.cpp, EUPL-1.2).
 */
#pragma once

#include <FastLED.h>

#include "DisplayLayout.h"


namespace EffectsNS
{
    /** @brief Size of the per-effect state buffer */
    static constexpr size_t mcDataSize = 1280;

    /** @brief Number and names of the available effects (for the web UI) */
    uint8_t GetEffectCount(void);
    const char* const* GetEffectNames(void);

    /** @brief Number and names of the available palettes (for the web UI) */
    uint8_t GetPaletteCount(void);
    const char* const* GetPaletteNames(void);


    /**
     * @brief Drawing canvas, parameters and runtime state of the running effect.
     * @details Mirrors the subset of the WLED segment API used by the ported effects.
     *          Coordinates: x = column (0..cols-1), y = row (0..rows-1), no zigzag.
     */
    struct Fx
    {
        static constexpr int mCols = MATRIX_WIDTH;
        static constexpr int mRows = MATRIX_HEIGHT;

        CRGB mLeds[MATRIX_WIDTH * MATRIX_HEIGHT];

        /* Parameters (WLED: speed, intensity, custom1..3, check1..3, palette) */
        uint8_t       mSpeed     = 128;
        uint8_t       mIntensity = 128;
        uint8_t       mCustom1   = 128;
        uint8_t       mCustom2   = 128;
        uint8_t       mCustom3   = 16;      // 0..31
        bool          mCheck1    = false;
        bool          mCheck2    = false;
        bool          mCheck3    = false;
        uint8_t       mPaletteIndex = 0;    // 0 = effect default
        CRGBPalette16 mPalette   = RainbowColors_p;

        /* Runtime state (WLED: strip.now, call, step, aux0, aux1, data) */
        uint32_t mNow  = 0;
        uint32_t mCall = 0;
        uint32_t mStep = 0;
        uint16_t mAux0 = 0;
        uint16_t mAux1 = 0;
        alignas(4) uint8_t mData[mcDataSize];

        /* Text for the scrolling text effect */
        char mText[12] = "";

        static bool Inside(int aX, int aY) { return (static_cast<unsigned>(aX) < mCols) && (static_cast<unsigned>(aY) < mRows); }
        static int  XY(int aX, int aY)     { return aX + aY * mCols; }

        void SetXY(int aX, int aY, const CRGB& arColor);
        CRGB GetXY(int aX, int aY) const;
        void AddXY(int aX, int aY, const CRGB& arColor);
        void BlendXY(int aX, int aY, const CRGB& arColor, uint8_t aAmount);
        void FadeXY(int aX, int aY, uint8_t aScale);

        void Fill(const CRGB& arColor);
        void FadeAll(uint8_t aAmount);
        CRGB Pal(uint8_t aIndex, uint8_t aBrightness = 255) const;

        void Blur(uint8_t aAmount);
        void BlurRows(uint8_t aAmount);
        void BlurCols(uint8_t aAmount);

        void DrawLine(int aX0, int aY0, int aX1, int aY1, const CRGB& arColor);
        void FillCircle(int aCx, int aCy, int aRadius, const CRGB& arColor);
        void WuPixel(uint32_t aX, uint32_t aY, const CRGB& arColor);   // 8.8 fixed point position
        void Move(uint8_t aDirection, uint8_t aDelta);                  // 0..7, clockwise from "left"

    private:
        void BlurLine(int aStart, int aStride, int aCount, uint8_t aAmount);
        void MoveX(int aDelta);
        void MoveY(int aDelta);
    };


    /** @brief Runs the selected effect on the canvas */
    class Engine
    {
    public:
        void Start(uint8_t aEffect);
        void SetParams(uint8_t aSpeed, uint8_t aIntensity, uint8_t aPalette);
        void SetText(const char* apText);
        void Render(uint32_t aNow);

        CRGB    GetPixel(int aX, int aY) const { return mFx.GetXY(aX, aY); }
        uint8_t GetEffect(void) const          { return mEffect; }

    private:
        Fx      mFx;
        uint8_t mEffect = 0xFF;
    };
}; /* end of namespace EffectsNS */
