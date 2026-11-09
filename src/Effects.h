/*
 * Effects.h
 *
 *  Created on: 09.11.2026
 *      Author: hocki
 *
 *  1D and 2D effects ported from WLED-MM (https://github.com/MoonModules/WLED-MM, wled00/FX.cpp, EUPL-1.2).
 */
#pragma once

#include <FastLED.h>

#include "DisplayLayout.h"


namespace EffectsNS
{
    /** @brief Size of the per-effect state buffer */
    static constexpr size_t mcDataSize = 4096;

    /** @brief Number and names (with 1D/2D mark) of the available effects (for the web UI) */
    uint8_t GetEffectCount(void);
    const char* const* GetEffectNames(void);

    /** @brief Number and names of the available palettes (for the web UI) */
    uint8_t GetPaletteCount(void);
    const char* const* GetPaletteNames(void);

    /** @brief Number and names of the 1D-to-2D mappings (for the web UI); 0 = effect default */
    uint8_t GetMappingCount(void);
    const char* const* GetMappingNames(void);

    /** @brief Effect types */
    enum tEffectType : uint8_t
    {
        TYPE_2D,        // Draws directly on the matrix
        TYPE_1D,        // Draws on a virtual strip, projected by the selected mapping
        TYPE_1D_COLS,   // WLED "1.5D": one virtual strip per matrix column
    };

    /** @brief 1D-to-2D mappings (WLED map1D2D) */
    enum tMapping : uint8_t
    {
        MAP_PIXELS,     // Strip runs row by row over the matrix
        MAP_BAR,        // Strip index = row from bottom, whole row lit
        MAP_CIRCLE,     // Strip index = distance from the matrix center
        MAP_CORNER,     // Strip index = square ring from the top-left corner
    };


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
        /* Virtual strip of 1D effects */
        CRGB mStrip[MATRIX_WIDTH * MATRIX_HEIGHT];

        tEffectType mType    = TYPE_2D;
        tMapping    mMapping = MAP_PIXELS;
        int         mLen     = MATRIX_WIDTH * MATRIX_HEIGHT;
        int         mStrips  = 1;

        /* Parameters (WLED: speed, intensity, custom1..3, check1..3, palette, colors) */
        uint8_t       mSpeed     = 128;
        uint8_t       mIntensity = 128;
        uint8_t       mCustom1   = 128;
        uint8_t       mCustom2   = 128;
        uint8_t       mCustom3   = 16;      // 0..31
        bool          mCheck1    = false;
        bool          mCheck2    = false;
        bool          mCheck3    = false;
        uint8_t       mPaletteIndex = 0;    // 0 = WLED "Default": colors instead of palette
        CRGBPalette16 mPalette   = PartyColors_p;
        CRGB          mColors[3] = { CRGB(0xFFAA00), CRGB::Black, CRGB::Black };

        /* Runtime state (WLED: strip.now, call, step, aux0, aux1, data, frame delay) */
        uint32_t mNow   = 0;
        uint32_t mCall  = 0;
        uint32_t mStep  = 0;
        uint16_t mAux0  = 0;
        uint16_t mAux1  = 0;
        uint16_t mDelay = 0;                // >0: next frame not before this many ms
        alignas(4) uint8_t mData[mcDataSize];

        /* Text for the scrolling text effect */
        char mText[12] = "";

        static bool Inside(int aX, int aY) { return (static_cast<unsigned>(aX) < mCols) && (static_cast<unsigned>(aY) < mRows); }
        static int  XY(int aX, int aY)     { return aX + aY * mCols; }

        /* 2D access */
        void SetXY(int aX, int aY, const CRGB& arColor);
        CRGB GetXY(int aX, int aY) const;
        void AddXY(int aX, int aY, const CRGB& arColor);
        void BlendXY(int aX, int aY, const CRGB& arColor, uint8_t aAmount);
        void FadeXY(int aX, int aY, uint8_t aScale);

        /* 1D access; index may carry a column strip number: i | ((strip + 1) << 16) */
        int  Len(void) const    { return mLen; }
        int  Strips(void) const { return mStrips; }
        static int VStrip(int aIndex, int aStrip) { return aIndex | ((aStrip + 1) << 16); }
        void SetPixel(int aIndex, const CRGB& arColor);
        CRGB GetPixel(int aIndex) const;
        void AddPixel(int aIndex, const CRGB& arColor);
        void BlendPixel(int aIndex, const CRGB& arColor, uint8_t aAmount);

        /* Whole buffer (strip for 1D effects, matrix otherwise) */
        void Fill(const CRGB& arColor);
        void FadeAll(uint8_t aAmount);          // WLED fadeToBlackBy()
        void FadeOut(uint8_t aRate);            // WLED fade_out(): towards color 2
        void Blur(uint8_t aAmount);
        void BlurRows(uint8_t aAmount);
        void BlurCols(uint8_t aAmount);

        /* Colors */
        CRGB Pal(uint8_t aIndex, uint8_t aBrightness = 255) const;
        CRGB ColorFromPal(uint32_t aIndex, bool aMapping, bool aWrap, uint8_t aColorSlot, uint8_t aBrightness = 255) const;
        CRGB Wheel(uint8_t aPos) const;
        uint8_t RandomWheelIndex(uint8_t aPos) const;

        void DrawLine(int aX0, int aY0, int aX1, int aY1, const CRGB& arColor);
        void DrawCircle(int aCx, int aCy, int aRadius, const CRGB& arColor);
        void FillCircle(int aCx, int aCy, int aRadius, const CRGB& arColor);
        void WuPixel(uint32_t aX, uint32_t aY, const CRGB& arColor);   // 8.8 fixed point position
        void Move(uint8_t aDirection, uint8_t aDelta, bool aWrap = false);  // 0..7 as WLED move()

        /* Projects the virtual strip of a 1D effect onto the matrix */
        void Project(void);

    private:
        CRGB* Buf(void)             { return (mType == TYPE_1D) ? mStrip : mLeds; }
        int   BufLen(void) const    { return (mType == TYPE_1D) ? mLen : (mCols * mRows); }
        void  BlurLine(CRGB* apBuf, int aStart, int aStride, int aCount, uint8_t aAmount);
        void  MoveX(int aDelta, bool aWrap);
        void  MoveY(int aDelta, bool aWrap);
    };


    /** @brief Effect description */
    using tEffectFn = void (*)(Fx&);

    struct tEffectInfo
    {
        const char* mName;
        tEffectFn   mFn;
        tEffectType mType;
        tMapping    mMapping;           // WLED default mapping (1D effects)
        uint8_t     mPalette;           // Palette used when "Default" is selected (0 = colors)
        uint8_t     mCustom1, mCustom2, mCustom3;
        bool        mCheck1, mCheck2, mCheck3;
    };

    /** @brief 1D effect table (Effects1D.cpp) */
    const tEffectInfo* GetEffects1D(uint8_t& arCount);


    /** @brief Runs the selected effect on the canvas */
    class Engine
    {
    public:
        void Start(uint8_t aEffect, uint8_t aMapping);
        void SetParams(uint8_t aSpeed, uint8_t aIntensity, uint8_t aPalette);
        void SetColors(const CRGB& arColor1, const CRGB& arColor2, const CRGB& arColor3);
        void SetText(const char* apText);
        void Render(uint32_t aNow);

        CRGB    GetPixel(int aX, int aY) const { return mFx.GetXY(aX, aY); }
        uint8_t GetEffect(void) const          { return mEffect; }
        uint8_t GetMapping(void) const         { return mMappingSetting; }

    private:
        void ApplyPalette(void);

        Fx       mFx;
        uint8_t  mEffect         = 0xFF;
        uint8_t  mMappingSetting = 0;
        uint8_t  mUserPalette    = 0;
        uint32_t mNextFrame      = 0;
    };
}; /* end of namespace EffectsNS */
