/*
 * Effects1D.cpp
 *
 *  Created on: 09.11.2026
 *      Author: hocki
 *
 *  1D effects (and effects drawing on 1D or 2D) ported from WLED-MM
 *  (https://github.com/MoonModules/WLED-MM, wled00/FX.cpp, EUPL-1.2).
 *  Original authors are named at the effects where known. Sound-reactive and particle-system effects are not included.
 */

#include <Arduino.h>
#include <algorithm>

#include "Effects.h"


namespace EffectsNS
{

/* WLED frame time in ms */
static constexpr uint32_t FRAMETIME = 24;
/* WLED paletteBlend = 0 */
static constexpr bool     PALETTE_SOLID_WRAP = false;

static inline bool     PaletteMovingWrap(const Fx& f) { return f.mSpeed != 0; }
static inline uint32_t SpeedFormulaL(const Fx& f)     { return 4U + (50U * (255U - f.mSpeed)) / std::min(f.Len(), 512); }

static uint8_t SinGap(uint16_t aIn)
{
    if (aIn & 0x100) return 0;
    return sin8(aIn + 192);
}

static uint16_t TriWave16(uint16_t aIn)
{
    if (aIn < 0x8000) return aIn * 2;
    return 0xFFFF - (aIn - 0x8000) * 2;
}

static int8_t TristateSquare8(uint8_t aX, uint8_t aPulseWidth, uint8_t aAttDec)
{
    int8_t wA = 127;
    if (aX > 127) { wA = -127; aX -= 127; }
    if (aX < aAttDec)                return int16_t(aX) * wA / aAttDec;
    if (aX < aPulseWidth - aAttDec)  return wA;
    if (aX < aPulseWidth)            return int16_t(aPulseWidth - aX) * wA / aAttDec;
    return 0;
}

/* Beat functions with an explicit time base (WLED drives FastLED beats with its own clock) */
static uint16_t Beat88(uint16_t aBpm88, uint32_t aMs)  { return (aMs * aBpm88 * 280) >> 16; }
static uint16_t Beat16(uint16_t aBpm, uint32_t aMs)    { if (aBpm < 256) aBpm <<= 8; return Beat88(aBpm, aMs); }
static uint8_t  Beat8(uint16_t aBpm, uint32_t aMs)     { return Beat16(aBpm, aMs) >> 8; }
static uint16_t BeatSin88(uint16_t aBpm88, uint16_t aLo, uint16_t aHi, uint32_t aMs)
{
    return aLo + scale16(sin16(Beat88(aBpm88, aMs)) + 32768, aHi - aLo);
}
static uint16_t BeatSin16(uint16_t aBpm, uint16_t aLo, uint16_t aHi, uint32_t aMs)
{
    return aLo + scale16(sin16(Beat16(aBpm, aMs)) + 32768, aHi - aLo);
}
static uint8_t BeatSin8(uint16_t aBpm, uint8_t aLo, uint8_t aHi, uint32_t aMs)
{
    return aLo + scale8(sin8(Beat8(aBpm, aMs)), aHi - aLo);
}

/* WLED beatsin8_t(bpm, -64, 64) relies on uint8 wrap-around */
static uint8_t BeatSin8Signed64(uint16_t aBpm)
{
    return static_cast<uint8_t>(beatsin8(aBpm, 0, 128) + 192);
}

static inline CRGB Blend(const CRGB& arA, const CRGB& arB, int aAmount)
{
    return blend(arA, arB, static_cast<uint8_t>(constrain(aAmount, 0, 255)));
}


/***************************************************************************************************
  Basic effects
 **************************************************************************************************/
static void FxSolid(Fx& f)
{
    f.Fill(f.mColors[0]);
}

static void BlinkBase(Fx& f, const CRGB& arColor1, const CRGB& arColor2, bool aStrobe, bool aPalette)
{
    uint32_t wCycle = (255 - f.mSpeed) * 20;
    uint32_t wOnTime = FRAMETIME;
    if (!aStrobe) wOnTime += (wCycle * f.mIntensity) >> 8;
    wCycle += FRAMETIME * 2;
    const uint32_t wIt  = f.mNow / wCycle;
    const uint32_t wRem = f.mNow % wCycle;
    const bool wOn = (wIt != f.mStep) || (wRem <= wOnTime);
    f.mStep = wIt;

    const CRGB wColor = wOn ? arColor1 : arColor2;
    if ((wColor == arColor1) && aPalette)
    {
        for (int wI = 0; wI < f.Len(); wI++) f.SetPixel(wI, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0));
    }
    else
    {
        f.Fill(wColor);
    }
}

static void FxBlink(Fx& f)         { BlinkBase(f, f.mColors[0], f.mColors[1], false, true); }
static void FxBlinkRainbow(Fx& f)  { BlinkBase(f, f.Wheel(f.mCall & 0xFF), f.mColors[1], false, false); }
static void FxStrobe(Fx& f)        { BlinkBase(f, f.mColors[0], f.mColors[1], true, true); }
static void FxStrobeRainbow(Fx& f) { BlinkBase(f, f.Wheel(f.mCall & 0xFF), f.mColors[1], true, false); }

static void ColorWipe(Fx& f, bool aRev, bool aRandom)
{
    const int wLen = f.Len();
    const uint32_t wCycle = 750 + (255 - f.mSpeed) * 150;
    const uint32_t wPerc  = f.mNow % wCycle;
    uint16_t wProg = (wPerc * 65535) / wCycle;
    const bool wBack = (wProg > 32767);
    if (wBack) { wProg -= 32767; if (f.mStep == 0) f.mStep = 1; }
    else       { if (f.mStep == 2) f.mStep = 3; }

    if (aRandom)
    {
        if (f.mCall == 0) { f.mAux0 = random8(); f.mStep = 3; }
        if (f.mStep == 1) { f.mAux1 = f.RandomWheelIndex(f.mAux0); f.mStep = 2; }
        if (f.mStep == 3) { f.mAux0 = f.RandomWheelIndex(f.mAux1); f.mStep = 0; }
    }

    const uint16_t wLedIndex = (uint32_t(wProg) * wLen) >> 15;
    uint16_t wRem = (uint32_t(wProg) * wLen) * 2;
    wRem /= (f.mIntensity + 1);
    if (wRem > 255) wRem = 255;

    const CRGB wCol1 = aRandom ? f.Wheel(f.mAux1) : f.mColors[1];
    for (int wI = 0; wI < wLen; wI++)
    {
        const int  wIndex = (aRev && wBack) ? (wLen - 1 - wI) : wI;
        const CRGB wCol0  = aRandom ? f.Wheel(f.mAux0) : f.ColorFromPal(wIndex, true, PALETTE_SOLID_WRAP, 0);
        if (wI < wLedIndex)
        {
            f.SetPixel(wIndex, wBack ? wCol1 : wCol0);
        }
        else
        {
            f.SetPixel(wIndex, wBack ? wCol0 : wCol1);
            if (wI == wLedIndex) f.SetPixel(wIndex, blend(wBack ? wCol0 : wCol1, wBack ? wCol1 : wCol0, wRem));
        }
    }
}

static void FxWipe(Fx& f)        { ColorWipe(f, false, false); }
static void FxSweep(Fx& f)       { ColorWipe(f, true,  false); }
static void FxWipeRandom(Fx& f)  { ColorWipe(f, false, true);  }
static void FxSweepRandom(Fx& f) { ColorWipe(f, true,  true);  }

/** @brief Calculates the linear, bounded progress used by the Random Colors fade. */
uint8_t FadeAmount(uint32_t aElapsedMs, uint32_t aDurationMs)
{
    if ((aDurationMs == 0) || (aElapsedMs >= aDurationMs)) return 255;
    return static_cast<uint8_t>((static_cast<uint64_t>(aElapsedMs) * 255U) / aDurationMs);
}

/** @brief Applies the Fade effect's color interpolation at the supplied progress. */
CRGB FadeColor(const CRGB& arFrom, const CRGB& arTo, uint8_t aAmount)
{
    return blend(arFrom, arTo, aAmount);
}

static void FxRandomColors(Fx& f)
{
    const uint32_t wCycle   = 200 + (255 - f.mSpeed) * 50;
    const uint32_t wIt      = f.mNow / wCycle;
    const uint32_t wRem     = f.mNow % wCycle;
    const uint16_t wFadeDur = (wCycle * f.mIntensity) >> 8;

    const uint8_t wFade = FadeAmount(wRem, wFadeDur);

    if (f.mCall == 0) { f.mAux0 = random8(); f.mStep = 2; }
    if (wIt != f.mStep)
    {
        f.mAux1 = f.mAux0;
        f.mAux0 = f.RandomWheelIndex(f.mAux0);
        f.mStep = wIt;
    }
    f.Fill(FadeColor(f.Wheel(f.mAux1), f.Wheel(f.mAux0), wFade));
}

static void DynamicBase(Fx& f, bool aSmooth)
{
    uint8_t* wData = f.mData;
    if (f.mCall == 0)
    {
        f.Fill(CRGB::Black);
        for (int wI = 0; wI < f.Len(); wI++) wData[wI] = random8();
    }

    const uint32_t wCycle = 50 + (255 - f.mSpeed) * 15;
    const uint32_t wIt    = f.mNow / wCycle;
    if ((wIt != f.mStep) && (f.mSpeed != 0))
    {
        for (int wI = 0; wI < f.Len(); wI++) if (random8() <= f.mIntensity) wData[wI] = random8();
        f.mStep = wIt;
    }

    if (aSmooth && (f.mCall > 1)) for (int wI = 0; wI < f.Len(); wI++) f.BlendPixel(wI, f.Wheel(wData[wI]), 16);
    else                          for (int wI = 0; wI < f.Len(); wI++) f.SetPixel(wI, f.Wheel(wData[wI]));
}

static void FxDynamic(Fx& f)       { DynamicBase(f, f.mCheck1); }
static void FxDynamicSmooth(Fx& f) { DynamicBase(f, true); }

static void FxBreathe(Fx& f)
{
    uint16_t wVar = 0;
    uint16_t wCounter = f.mNow * ((f.mSpeed >> 3) + 10);
    wCounter = (wCounter >> 2) + (wCounter >> 4);
    if (wCounter < 16384)
    {
        if (wCounter > 8192) wCounter = 8192 - (wCounter - 8192);
        wVar = sin16(wCounter) / 103;
    }
    const uint8_t wLum = 30 + wVar;
    for (int wI = 0; wI < f.Len(); wI++) f.SetPixel(wI, blend(f.mColors[1], f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0), wLum));
}

static void FxFade(Fx& f)
{
    const uint16_t wCounter = f.mNow * ((f.mSpeed >> 3) + 10);
    const uint8_t  wLum     = TriWave16(wCounter) >> 8;
    for (int wI = 0; wI < f.Len(); wI++) f.SetPixel(wI, FadeColor(f.mColors[1], f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0), wLum));
}

static void ScanBase(Fx& f, bool aDual)
{
    const int wLen = f.Len();
    const uint32_t wCycle = 750 + (255 - f.mSpeed) * 150;
    const uint32_t wPerc  = f.mNow % wCycle;
    const uint16_t wProg  = (wPerc * 65535) / wCycle;
    const int      wSize  = 1 + ((f.mIntensity * wLen) >> 9);
    const int      wLedIndex = (uint32_t(wProg) * ((wLen * 2) - wSize * 2)) >> 16;

    if (!f.mCheck2) f.Fill(f.mColors[1]);

    const int wOffset = abs(wLedIndex - (wLen - wSize));
    if (aDual)
    {
        for (int wJ = wOffset; wJ < wOffset + wSize; wJ++)
        {
            const int wI2 = wLen - 1 - wJ;
            f.SetPixel(wI2, f.ColorFromPal(wI2, true, PALETTE_SOLID_WRAP, f.mColors[2] ? 2 : 0));
        }
    }
    for (int wJ = wOffset; wJ < wOffset + wSize; wJ++) f.SetPixel(wJ, f.ColorFromPal(wJ, true, PALETTE_SOLID_WRAP, 0));
}

static void FxScan(Fx& f)     { ScanBase(f, false); }
static void FxScanDual(Fx& f) { ScanBase(f, true); }

static void FxColorloop(Fx& f)
{
    uint16_t wCounter = (f.mNow * ((f.mSpeed >> 2) + 2)) & 0xFFFF;
    wCounter >>= 8;
    if (f.mIntensity < 128) f.Fill(blend(f.Wheel(wCounter), CRGB(CRGB::White), 128 - f.mIntensity));
    else                    f.Fill(f.Wheel(wCounter));
}

static void FxRainbow(Fx& f)
{
    uint16_t wCounter = (f.mNow * ((f.mSpeed >> 2) + 2)) & 0xFFFF;
    wCounter >>= 8;
    for (int wI = 0; wI < f.Len(); wI++)
    {
        const uint8_t wIndex = (wI * (16 << (f.mIntensity / 29)) / f.Len()) + wCounter;
        f.SetPixel(wI, f.Wheel(wIndex));
    }
}

static void RunningBase(Fx& f, CRGB aColor1, const CRGB& arColor2, bool aTheatre)
{
    const uint8_t  wWidth = (aTheatre ? 3 : 1) + (f.mIntensity >> 4);
    const uint32_t wCycle = 50 + (255 - f.mSpeed);
    const uint32_t wIt    = f.mNow / wCycle;
    const bool wUsePalette = (aColor1 == f.mColors[0]);

    for (int wI = 0; wI < f.Len(); wI++)
    {
        CRGB wCol = arColor2;
        if (wUsePalette) aColor1 = f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0);
        if (aTheatre)
        {
            if ((wI % wWidth) == f.mAux0) wCol = aColor1;
        }
        else
        {
            const int8_t wPos = wI % (wWidth << 1);
            if ((wPos < int(f.mAux0) - wWidth) || ((wPos >= f.mAux0) && (wPos < f.mAux0 + wWidth))) wCol = aColor1;
        }
        f.SetPixel(wI, wCol);
    }

    if (wIt != f.mStep)
    {
        f.mAux0 = (f.mAux0 + 1) % (aTheatre ? wWidth : (wWidth << 1));
        f.mStep = wIt;
    }
}

static void FxTheater(Fx& f)        { RunningBase(f, f.mColors[0], f.mColors[1], true); }
static void FxTheaterRainbow(Fx& f) { RunningBase(f, f.Wheel(f.mStep), f.mColors[1], true); }
static void FxChase2(Fx& f)         { RunningBase(f, f.mColors[0], f.mColors[1], false); }

static void RunningLightsBase(Fx& f, bool aSaw, bool aDual)
{
    const uint8_t  wXScale  = f.mIntensity >> 2;
    const uint32_t wCounter = (f.mNow * f.mSpeed) >> 9;

    for (int wI = 0; wI < f.Len(); wI++)
    {
        uint16_t wA = wI * wXScale - wCounter;
        if (aSaw)
        {
            wA &= 0xFF;
            if (wA < 16) wA = 192 + wA * 8;
            else         wA = map(wA, 16, 255, 64, 192);
            wA = 255 - wA;
        }
        const uint8_t wS = aDual ? SinGap(wA) : sin8(wA);
        CRGB wCa = blend(f.mColors[1], f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0), wS);
        if (aDual)
        {
            const uint16_t wB = (f.Len() - 1 - wI) * wXScale - wCounter;
            const uint8_t  wT = SinGap(wB);
            const CRGB wCb = blend(f.mColors[1], f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 2), wT);
            wCa = blend(wCa, wCb, 127);
        }
        f.SetPixel(wI, wCa);
    }
}

static void FxRunning(Fx& f)     { RunningLightsBase(f, false, false); }
static void FxRunningDual(Fx& f) { RunningLightsBase(f, false, true); }
static void FxSaw(Fx& f)         { RunningLightsBase(f, true, false); }

static void FxTwinkle(Fx& f)
{
    f.FadeOut(224);

    const uint32_t wCycle = 20 + (255 - f.mSpeed) * 5;
    const uint32_t wIt    = f.mNow / wCycle;
    if (wIt != f.mStep)
    {
        const uint16_t wMaxOn = map(f.mIntensity, 0, 255, 1, f.Len());
        if (f.mAux0 >= wMaxOn) { f.mAux0 = 0; f.mAux1 = random16(); }
        f.mAux0++;
        f.mStep = wIt;
    }

    uint16_t wPrng = f.mAux1;
    for (uint16_t wI = 0; wI < f.mAux0; wI++)
    {
        wPrng = uint16_t(wPrng * 2053) + 13849;
        const uint32_t wP = uint32_t(f.Len()) * wPrng;
        const int wJ = wP >> 16;
        f.SetPixel(wJ, f.ColorFromPal(wJ, true, PALETTE_SOLID_WRAP, 0));
    }
}

static void DissolveBase(Fx& f, const CRGB& arColor)
{
    if (f.mCall == 0) f.Fill(f.mColors[1]);

    for (int wJ = 0; wJ <= f.Len() / 15; wJ++)
    {
        if (random8() > f.mIntensity) continue;
        for (int wTimes = 0; wTimes < 10; wTimes++)
        {
            const int wI = random16(f.Len());
            if (f.mAux0)
            {
                if (f.GetPixel(wI) == f.mColors[1])
                {
                    f.SetPixel(wI, (arColor == f.mColors[0]) ? f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0) : arColor);
                    break;
                }
            }
            else if (f.GetPixel(wI) != f.mColors[1])
            {
                f.SetPixel(wI, f.mColors[1]);
                break;
            }
        }
    }

    if (f.mStep > (255U - f.mSpeed) + 15U) { f.mAux0 = !f.mAux0; f.mStep = 0; }
    else                                     f.mStep++;
}

static void FxDissolve(Fx& f)    { DissolveBase(f, f.mCheck1 ? f.Wheel(random8()) : f.mColors[0]); }
static void FxDissolveRnd(Fx& f) { DissolveBase(f, f.Wheel(random8())); }

static void FxSparkle(Fx& f)
{
    if (!f.mCheck2) for (int wI = 0; wI < f.Len(); wI++) f.SetPixel(wI, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 1));
    const uint32_t wCycle = 10 + (255 - f.mSpeed) * 2;
    const uint32_t wIt    = f.mNow / wCycle;
    if (wIt != f.mStep) { f.mAux0 = random16(f.Len()); f.mStep = wIt; }
    f.SetPixel(f.mAux0, f.mColors[0]);
}

static void SparkleDarkBase(Fx& f, bool aHyper)
{
    if (!f.mCheck2) for (int wI = 0; wI < f.Len(); wI++) f.SetPixel(wI, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0));

    if ((f.mNow - f.mAux0) > f.mStep)
    {
        if (random8((255 - f.mIntensity) >> 4) == 0)
        {
            const int wCount = aHyper ? std::max(1, f.Len() / 3) : 1;
            for (int wI = 0; wI < wCount; wI++) f.SetPixel(random16(f.Len()), f.mColors[1]);
        }
        f.mStep = f.mNow;
        f.mAux0 = 255 - f.mSpeed;
    }
}

static void FxSparkleDark(Fx& f)  { SparkleDarkBase(f, false); }
static void FxSparklePlus(Fx& f)  { SparkleDarkBase(f, true); }

static void FxStrobeMega(Fx& f)
{
    for (int wI = 0; wI < f.Len(); wI++) f.SetPixel(wI, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 1));

    f.mAux0 = 50 + 20 * uint16_t(255 - f.mSpeed);
    const uint16_t wCount = 2 * ((f.mIntensity / 10) + 1);
    if (f.mAux1 < wCount)
    {
        if ((f.mAux1 & 1) == 0) { f.Fill(f.mColors[0]); f.mAux0 = 15; }
        else                      f.mAux0 = 50;
    }
    if ((f.mNow - f.mAux0) > f.mStep)
    {
        f.mAux1++;
        if (f.mAux1 > wCount) f.mAux1 = 0;
        f.mStep = f.mNow;
    }
}

static void FxAndroid(Fx& f)
{
    const int wLen = f.Len();
    uint32_t* wCounter = reinterpret_cast<uint32_t*>(f.mData);
    unsigned wSize      = f.mAux1 >> 1;
    unsigned wShrinking = f.mAux1 & 0x01;

    if (f.mNow >= f.mStep)
    {
        f.mStep = f.mNow + 3 + ((8 * uint32_t(255 - f.mSpeed)) / wLen);
        if (wSize > unsigned(f.mIntensity * wLen) / 255) wShrinking = 1;
        else if (wSize < 2)                             wShrinking = 0;
        if (!wShrinking)
        {
            if ((*wCounter % 3) == 1) f.mAux0++;
            else                      wSize++;
        }
        else
        {
            f.mAux0++;
            if ((*wCounter % 3) != 1) wSize--;
        }
        f.mAux1 = (wSize << 1) | wShrinking;
        (*wCounter)++;
        if (f.mAux0 >= wLen) f.mAux0 = 0;
    }

    const int wStart = f.mAux0;
    const int wEnd   = (f.mAux0 + wSize) % wLen;
    for (int wI = 0; wI < wLen; wI++)
    {
        if (((wStart < wEnd) && (wI >= wStart) && (wI < wEnd)) || ((wStart >= wEnd) && ((wI >= wStart) || (wI < wEnd))))
            f.SetPixel(wI, f.mColors[0]);
        else
            f.SetPixel(wI, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 1));
    }
}

static void ChaseBase(Fx& f, CRGB aColor1, const CRGB& arColor2, const CRGB& arColor3, bool aPalette, bool aRandom)
{
    const int wLen = f.Len();
    const uint16_t wCounter = f.mNow * ((f.mSpeed >> 2) + 1);
    const int wA = (uint32_t(wCounter) * wLen) >> 16;

    if (aRandom)
    {
        if (wA < int(f.mStep)) { f.mAux1 = f.mAux0; f.mAux0 = f.RandomWheelIndex(f.mAux0); }
        aColor1 = f.Wheel(f.mAux0);
    }
    f.mStep = wA;

    const int wSize = 1 + ((f.mIntensity * wLen) >> 10);
    int wB = wA + wSize; if (wB > wLen) wB -= wLen;
    int wC = wB + wSize; if (wC > wLen) wC -= wLen;

    if (aPalette) for (int wI = 0; wI < wLen; wI++) f.SetPixel(wI, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 1));
    else          f.Fill(aColor1);

    if (aRandom)
    {
        aColor1 = f.Wheel(f.mAux1);
        for (int wI = wA; wI < wLen; wI++) f.SetPixel(wI, aColor1);
    }

    if (wA < wB) { for (int wI = wA; wI < wB; wI++) f.SetPixel(wI, arColor2); }
    else         { for (int wI = wA; wI < wLen; wI++) f.SetPixel(wI, arColor2); for (int wI = 0; wI < wB; wI++) f.SetPixel(wI, arColor2); }

    if (wB < wC) { for (int wI = wB; wI < wC; wI++) f.SetPixel(wI, arColor3); }
    else         { for (int wI = wB; wI < wLen; wI++) f.SetPixel(wI, arColor3); for (int wI = 0; wI < wC; wI++) f.SetPixel(wI, arColor3); }
}

static void FxChase(Fx& f)
{
    ChaseBase(f, f.mColors[1], f.mColors[2] ? f.mColors[2] : f.mColors[0], f.mColors[0], true, false);
}

static void FxChaseRandom(Fx& f)
{
    ChaseBase(f, f.mColors[1], f.mColors[2] ? f.mColors[2] : f.mColors[0], f.mColors[0], false, true);
}

static void FxChaseRainbow(Fx& f)
{
    uint8_t wSep = 256 / f.Len();
    if (wSep == 0) wSep = 1;
    const uint8_t wIdx = f.mCall & 0xFF;
    const CRGB wColor = f.Wheel(((f.mStep * wSep) + wIdx) & 0xFF);
    ChaseBase(f, wColor, f.mColors[0], f.mColors[1], false, false);
}

static void FxRainbowRunner(Fx& f)
{
    const int wN = f.mStep;
    const int wM = (f.mStep + 1) % f.Len();
    const CRGB wColor2 = f.Wheel(((wN * 256 / f.Len()) + (f.mCall & 0xFF)) & 0xFF);
    const CRGB wColor3 = f.Wheel(((wM * 256 / f.Len()) + (f.mCall & 0xFF)) & 0xFF);
    ChaseBase(f, f.mColors[0], wColor2, wColor3, false, false);
}

static void FxColorful(Fx& f)
{
    uint8_t wNum = 4;
    CRGB wCols[9] = { CRGB(0xFF0000), CRGB(0xEEBB00), CRGB(0x00EE00), CRGB(0x0077CC) };

    if ((f.mIntensity > 160) || f.mPaletteIndex)
    {
        if (!f.mPaletteIndex)
        {
            wNum = 3;
            for (int wI = 0; wI < 3; wI++) wCols[wI] = f.mColors[wI];
        }
        else
        {
            for (int wI = 0; wI < wNum; wI++) wCols[wI] = f.ColorFromPal(wI * 80, false, true, 255);
        }
    }
    else if (f.mIntensity < 80)
    {
        wCols[0] = CRGB(0xFF8040); wCols[1] = CRGB(0xE5D241); wCols[2] = CRGB(0x77FF77); wCols[3] = CRGB(0x77F0F0);
    }
    for (int wI = wNum; wI < wNum * 2 - 1; wI++) wCols[wI] = wCols[wI - wNum];

    const uint32_t wCycle = 50 + (8 * uint32_t(255 - f.mSpeed));
    const uint32_t wIt    = f.mNow / wCycle;
    if (wIt != f.mStep)
    {
        if (f.mSpeed > 0) f.mAux0++;
        if (f.mAux0 >= wNum) f.mAux0 = 0;
        f.mStep = wIt;
    }
    for (int wI = 0; wI < f.Len(); wI += wNum)
    {
        for (int wJ = 0; wJ < wNum; wJ++) f.SetPixel(wI + wJ, wCols[f.mAux0 + wJ]);
    }
}

static void FxTrafficLight(Fx& f)
{
    for (int wI = 0; wI < f.Len(); wI++) f.SetPixel(wI, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 1));

    uint32_t wDelay = 500;
    for (int wI = 0; wI < f.Len() - 2; wI += 3)
    {
        switch (f.mAux0)
        {
            case 0: f.SetPixel(wI, CRGB(0xFF0000)); wDelay = 150 + (100 * uint32_t(255 - f.mSpeed)); break;
            case 1: f.SetPixel(wI, CRGB(0xFF0000)); wDelay = 150 + (20 * uint32_t(255 - f.mSpeed)); f.SetPixel(wI + 1, CRGB(0xEECC00)); break;
            case 2: f.SetPixel(wI + 2, CRGB(0x00FF00)); wDelay = 150 + (100 * uint32_t(255 - f.mSpeed)); break;
            case 3: f.SetPixel(wI + 1, CRGB(0xEECC00)); wDelay = 150 + (20 * uint32_t(255 - f.mSpeed)); break;
        }
    }
    if ((f.mNow - f.mStep) > wDelay)
    {
        f.mAux0++;
        if ((f.mAux0 == 1) && (f.mIntensity > 140)) f.mAux0 = 2;
        if (f.mAux0 > 3) f.mAux0 = 0;
        f.mStep = f.mNow;
    }
}

static void FxChaseFlash(Fx& f)
{
    const uint8_t wFlashStep = f.mCall % ((4 * 2) + 1);
    for (int wI = 0; wI < f.Len(); wI++) f.SetPixel(wI, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0));

    uint16_t wDelay = 10 + ((30 * uint16_t(255 - f.mSpeed)) / f.Len());
    if (wFlashStep < 8)
    {
        if ((wFlashStep % 2) == 0)
        {
            f.SetPixel(f.mStep, f.mColors[1]);
            f.SetPixel((f.mStep + 1) % f.Len(), f.mColors[1]);
            wDelay = 20;
        }
        else
        {
            wDelay = 30;
        }
    }
    else
    {
        f.mStep = (f.mStep + 1) % f.Len();
    }
    f.mDelay = wDelay;
}

static void FxChaseFlashRnd(Fx& f)
{
    const uint8_t wFlashStep = f.mCall % ((4 * 2) + 1);
    for (int wI = 0; wI < f.mAux1; wI++) f.SetPixel(wI, f.Wheel(f.mAux0));

    uint16_t wDelay = 1 + ((10 * uint16_t(255 - f.mSpeed)) / f.Len());
    if (wFlashStep < 8)
    {
        const int wN = f.mAux1;
        const int wM = (f.mAux1 + 1) % f.Len();
        if ((wFlashStep % 2) == 0) { f.SetPixel(wN, f.mColors[0]); f.SetPixel(wM, f.mColors[0]); wDelay = 20; }
        else                       { f.SetPixel(wN, f.Wheel(f.mAux0)); f.SetPixel(wM, f.mColors[1]); wDelay = 30; }
    }
    else
    {
        f.mAux1 = (f.mAux1 + 1) % f.Len();
        if (f.mAux1 == 0) f.mAux0 = f.RandomWheelIndex(f.mAux0);
    }
    f.mDelay = wDelay;
}

static void FxStream(Fx& f)
{
    const uint32_t wCycle = 25 + (3 * uint32_t(255 - f.mSpeed));
    const uint32_t wIt    = f.mNow / wCycle;
    if (f.mCall == 0) f.mAux0 = random16();

    const uint8_t wZoneSize = ((255 - f.mIntensity) >> 4) + 1;
    uint16_t wPrng = f.mAux0;
    uint8_t  wZ = wIt % wZoneSize;
    bool     wNZone = (!wZ && (uint16_t(wIt) != f.mAux1));

    for (int wI = f.Len() - 1; wI >= 0; wI--)
    {
        if (wNZone || (wZ >= wZoneSize))
        {
            const uint8_t wLastRand = wPrng >> 8;
            int16_t wDiff = 0;
            while (abs(wDiff) < 42)
            {
                wPrng = uint16_t(wPrng * 2053) + 13849;
                wDiff = (wPrng >> 8) - wLastRand;
            }
            if (wNZone) { f.mAux0 = wPrng; wNZone = false; }
            wZ = 0;
        }
        f.SetPixel(wI, f.Wheel(wPrng >> 8));
        wZ++;
    }
    f.mAux1 = wIt;
}

static void LarsonBase(Fx& f, bool aDual)
{
    const int wLen = f.Len();
    const uint16_t wCounter = f.mNow * ((f.mSpeed >> 2) + 8);
    const int wIndex = (uint32_t(wCounter) * wLen) >> 16;
    if (f.mCall == 0) f.Fill(CRGB::Black);

    f.FadeOut(f.mIntensity);

    if ((int(f.mStep) > wIndex) && ((int(f.mStep) - wIndex) > wLen / 2)) f.mAux0 = !f.mAux0;

    for (int wI = f.mStep; wI < wIndex; wI++)
    {
        const int wJ = f.mAux0 ? wI : (wLen - 1 - wI);
        f.SetPixel(wJ, f.ColorFromPal(wJ, true, PALETTE_SOLID_WRAP, 0));
    }
    if (aDual)
    {
        const CRGB wC = f.mColors[2] ? f.mColors[2] : f.ColorFromPal(wIndex, true, PALETTE_SOLID_WRAP, 0);
        for (int wI = f.mStep; wI < wIndex; wI++)
        {
            const int wJ = f.mAux0 ? (wLen - 1 - wI) : wI;
            f.SetPixel(wJ, wC);
        }
    }
    f.mStep = wIndex;
}

static void FxScanner(Fx& f)     { LarsonBase(f, false); }
static void FxScannerDual(Fx& f) { LarsonBase(f, true); }

static void FxLighthouse(Fx& f)
{
    const int wLen = f.Len();
    const uint16_t wCounter = f.mNow * ((f.mSpeed >> 2) + 1);
    int wIndex = (uint32_t(wCounter) * wLen) >> 16;
    if (f.mCall == 0) { f.Fill(CRGB::Black); f.mAux0 = wIndex; }

    f.FadeOut(f.mIntensity);
    f.SetPixel(wIndex, f.ColorFromPal(wIndex, true, PALETTE_SOLID_WRAP, 0));
    if (wIndex > f.mAux0)
    {
        for (int wI = f.mAux0; wI < wIndex; wI++) f.SetPixel(wI, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0));
    }
    else if ((wIndex < f.mAux0) && (wIndex < 10))
    {
        for (int wI = 0; wI < wIndex; wI++) f.SetPixel(wI, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0));
    }
    f.mAux0 = wIndex;
}


/***************************************************************************************************
  Effects drawing on the matrix (WLED "1D/2D" effects)
 **************************************************************************************************/
/* Fireworks - matrix variant */
static void FireworksCore(Fx& f)
{
    const int wW = Fx::mCols, wH = Fx::mRows;
    if (f.mCall == 0) { f.Fill(f.mColors[1]); f.mAux0 = 0xFFFF; f.mAux1 = 0xFFFF; }
    f.FadeOut(128);

    const bool wValid1 = f.mAux0 < (wW * wH);
    const bool wValid2 = f.mAux1 < (wW * wH);
    CRGB wSv1, wSv2;
    if (wValid1) wSv1 = f.GetXY(f.mAux0 % wW, f.mAux0 / wW);
    if (wValid2) wSv2 = f.GetXY(f.mAux1 % wW, f.mAux1 / wW);
    if (!f.mStep) f.Blur(16);
    if (wValid1) f.SetXY(f.mAux0 % wW, f.mAux0 / wW, wSv1);
    if (wValid2) f.SetXY(f.mAux1 % wW, f.mAux1 / wW, wSv2);

    const unsigned wIntensity = 129 - (f.mIntensity >> 1);
    for (int wI = 0; wI < std::max(1, wW / 20); wI++)
    {
        if (random8(wIntensity) == 0)
        {
            const uint16_t wIndex = random16(wW * wH);
            f.SetXY(wIndex % wW, wIndex / wW, f.ColorFromPal(random8(), false, false, 0));
            f.mAux1 = f.mAux0;
            f.mAux0 = wIndex;
        }
    }
}

static void FxFireworks(Fx& f)
{
    FireworksCore(f);
}

static void FxRain(Fx& f)
{
    const int wW = Fx::mCols, wH = Fx::mRows;
    if (f.mCall == 0) f.Fill(CRGB::Black);

    f.mStep += FRAMETIME;
    if (f.mCall && (f.mStep > SpeedFormulaL(f)))
    {
        f.mStep = 1;
        f.Move(6, 1, true);
        f.mAux0 = (f.mAux0 % wW) + (f.mAux0 / wW + 1) * wW;
        f.mAux1 = (f.mAux1 % wW) + (f.mAux1 / wW + 1) * wW;
        if (f.mAux0 >= wW * wH) f.mAux0 = 0;
        if (f.mAux1 >= wW * wH) f.mAux1 = 0;
    }
    FireworksCore(f);
}

/* Ripple - matrix variant */
struct tRipple { uint8_t mState; uint8_t mColor; uint16_t mPos; };

static void RippleBase(Fx& f)
{
    const int wCols = Fx::mCols, wRows = Fx::mRows;
    const int wMaxRipples = std::min(1 + ((wCols * wRows) >> 2), 100);
    const int wMaxDim = std::max(2, (wCols + wRows) / 4);
    tRipple* wRipples = reinterpret_cast<tRipple*>(f.mData);

    for (int wI = 0; wI < wMaxRipples; wI++)
    {
        uint16_t wState = wRipples[wI].mState;
        if (wState)
        {
            const uint8_t  wDecay  = (f.mSpeed >> 4) + 1;
            const uint16_t wOrigin = wRipples[wI].mPos;
            const CRGB     wCol    = f.ColorFromPal(wRipples[wI].mColor, false, false, 255);
            const uint16_t wProp   = ((wState / wDecay - 1) * (f.mSpeed + 1));
            int            wPropI  = wProp >> 8;
            const uint8_t  wPropF  = wProp & 0xFF;
            const uint8_t  wAmp    = (wState < 17) ? triwave8((wState - 1) * 8) : map(wState, 17, 255, 255, 2);

            wPropI /= 2;
            const int wCx = wOrigin >> 8;
            const int wCy = wOrigin & 0xFF;
            const uint8_t wMag = scale8(sin8(wPropF >> 2), wAmp);
            wPropI = std::min(wPropI, wMaxDim);
            if ((wPropI > 0) && ((wCx + wPropI) < wCols) && (wCy < wRows))
            {
                f.DrawCircle(wCx, wCy, wPropI, blend(f.GetXY(wCx + wPropI, wCy), wCol, wMag));
            }
            wState += wDecay;
            wRipples[wI].mState = (wState > 254) ? 0 : wState;
        }
        else if (random16(5100 + 10000) <= (f.mIntensity >> 3))
        {
            wRipples[wI].mState = 1;
            wRipples[wI].mPos   = (random16(wCols) << 8) | random16(wRows);
            wRipples[wI].mColor = random8();
        }
    }
}

static void FxRipple(Fx& f)
{
    if (!f.mCheck2) f.Fill(f.mColors[1]);
    else            f.FadeOut(250);
    RippleBase(f);
}

static void FxRippleRainbow(Fx& f)
{
    if (f.mCall == 0) { f.mAux0 = random8(); f.mAux1 = random8(); }
    if (f.mAux0 == f.mAux1)    f.mAux1 = random8();
    else if (f.mAux1 > f.mAux0) f.mAux0++;
    else                        f.mAux0--;
    f.Fill(blend(f.Wheel(f.mAux0), CRGB(CRGB::Black), 235));
    RippleBase(f);
}

/* Halloween Eyes - matrix variant */
static void FxHalloweenEyes(Fx& f)
{
    uint16_t* wStateTime = reinterpret_cast<uint16_t*>(f.mData);
    uint16_t* wRow       = wStateTime + 1;
    const int wMaxWidth  = Fx::mCols;
    const int wSpace     = std::max(2, Fx::mCols >> 4);
    const int wWidth     = wSpace / 2;
    const int wEyeLength = (2 * wWidth) + wSpace;

    if (!f.mCheck2) f.Fill(f.mColors[1]);

    uint8_t wState = f.mAux1 >> 8;
    if (*wStateTime == 0) *wStateTime = 2000;

    if (wState == 0)
    {
        f.mAux0 = random16(0, wMaxWidth - wEyeLength - 1);
        f.mAux1 = random8();
        *wRow   = random16(Fx::mRows - 1);
        wState  = 1;
    }

    if (wState < 2)
    {
        const int wStart2 = f.mAux0 + wWidth + wSpace;
        uint32_t wFade = (f.mNow - f.mStep) * 255 / *wStateTime;
        if (wFade > 255) wFade = 255;
        const CRGB wC = blend(f.ColorFromPal(f.mAux1 & 0xFF, false, false, 0), f.mColors[1], wFade);
        for (int wI = 0; wI < wWidth; wI++)
        {
            f.SetXY(f.mAux0 + wI, *wRow, wC);
            f.SetXY(wStart2 + wI, *wRow, wC);
        }
    }

    if ((f.mNow - f.mStep) > *wStateTime)
    {
        wState++;
        if (wState > 2) wState = 0;
        if (wState < 2) *wStateTime = 100 + f.mIntensity * 10;
        else
        {
            const uint16_t wBase = (256 - f.mSpeed) * 10;
            *wStateTime = wBase + random16(wBase);
        }
        f.mStep = f.mNow;
    }
    f.mAux1 = (f.mAux1 & 0xFF) + (wState << 8);
}

/* Exploding fireworks - by Blaz Kristan, matrix variant */
struct tSpark { float mPos, mPosX, mVel, mVelX; uint16_t mCol; uint8_t mColIndex; };

static void FxFireworks1D(Fx& f)
{
    const int wCols = Fx::mCols, wRows = Fx::mRows;
    if (f.mCall == 0) f.Fill(CRGB::Black);

    const unsigned wMaxSparks = (mcDataSize - sizeof(float)) / sizeof(tSpark);
    const unsigned wNumSparks = std::min<unsigned>(5 + ((wRows * wCols) >> 1), wMaxSparks);
    const unsigned wDataSize  = sizeof(tSpark) * wNumSparks;
    float* wDyingGravity = reinterpret_cast<float*>(f.mData + wDataSize);

    if (wDataSize != f.mAux1) { *wDyingGravity = 0.0f; f.mAux0 = 0; f.mAux1 = wDataSize; }

    f.FadeOut(252);

    tSpark* wSparks = reinterpret_cast<tSpark*>(f.mData);
    tSpark* wFlare  = wSparks;
    float wGravity = -0.0004f - (f.mSpeed / 800000.0f);
    wGravity *= wRows;

    if (f.mAux0 < 2)
    {
        if (f.mAux0 == 0)
        {
            wFlare->mPos  = 0;
            wFlare->mPosX = random16(2, wCols - 3);
            uint16_t wPeak = 75 + random8(180);
            wPeak = (wPeak * (wRows - 1)) >> 8;
            wFlare->mVel  = sqrtf(-2.0f * wGravity * wPeak);
            wFlare->mVelX = (int(random8(9)) - 4) / 32.f;
            wFlare->mCol  = 255;
            f.mAux0 = 1;
        }

        if (wFlare->mVel > 12 * wGravity)
        {
            const uint8_t wB = wFlare->mCol;
            f.SetXY(int(wFlare->mPosX), wRows - int(wFlare->mPos) - 1, CRGB(wB, wB, wB));
            wFlare->mPos  += wFlare->mVel;
            wFlare->mPosX += wFlare->mVelX;
            wFlare->mPos   = constrain(wFlare->mPos, 0.0f, float(wRows - 1));
            wFlare->mPosX  = constrain(wFlare->mPosX, 0.0f, float(wCols - 1));
            wFlare->mVel  += wGravity;
            wFlare->mCol  -= 2;
        }
        else
        {
            f.mAux0 = 2;
        }
    }
    else if (f.mAux0 < 4)
    {
        unsigned wN = wFlare->mPos + random8(4);
        wN = std::max(wN, 4U);
        wN = std::min(wN, wNumSparks);

        if (f.mAux0 == 2)
        {
            for (unsigned wI = 1; wI < wN; wI++)
            {
                wSparks[wI].mPos   = wFlare->mPos;
                wSparks[wI].mPosX  = wFlare->mPosX;
                wSparks[wI].mVel   = (float(random16(20001)) / 10000.0f) - 0.9f;
                wSparks[wI].mVel  *= (wRows < 32) ? 0.5f : 1;
                wSparks[wI].mVelX  = (float(random16(10001)) / 10000.0f) - 0.5f;
                wSparks[wI].mCol   = 345;
                wSparks[wI].mColIndex = random8();
                wSparks[wI].mVel  *= wFlare->mPos / wRows;
                wSparks[wI].mVelX *= wFlare->mPosX / wCols;
                wSparks[wI].mVel  *= -wGravity * 50.0f;
            }
            *wDyingGravity = wGravity / 2.0f;
            f.mAux0 = 3;
        }

        if (wSparks[1].mCol > 4)
        {
            for (unsigned wI = 1; wI < wN; wI++)
            {
                tSpark& wS = wSparks[wI];
                wS.mPos  += wS.mVel;
                wS.mPosX += wS.mVelX;
                wS.mVel  += *wDyingGravity;
                wS.mVelX += *wDyingGravity;
                if (wS.mCol > 3) wS.mCol -= 4;

                if ((wS.mPos > 0) && (wS.mPos < wRows))
                {
                    if (!((wS.mPosX >= 0) && (wS.mPosX < wCols))) continue;
                    const uint16_t wProg = wS.mCol;
                    const CRGB wSpColor = f.mPaletteIndex ? f.Wheel(wS.mColIndex) : f.mColors[0];
                    CRGB wC = CRGB::Black;
                    if (wProg > 300)
                    {
                        wC = Blend(wSpColor, CRGB(CRGB::White), (wProg - 300) * 5);
                    }
                    else if (wProg > 45)
                    {
                        wC = Blend(CRGB(CRGB::Black), wSpColor, wProg - 45);
                        const uint8_t wCooling = (300 - wProg) >> 5;
                        wC.g = qsub8(wC.g, wCooling);
                        wC.b = qsub8(wC.b, wCooling * 2);
                    }
                    f.SetXY(int(wS.mPosX), wRows - int(wS.mPos) - 1, wC);
                }
            }
            f.Blur(16);
            *wDyingGravity *= .8f;
        }
        else
        {
            f.mAux0 = 6 + random8(10);
        }
    }
    else
    {
        f.mAux0--;
        if (f.mAux0 < 4) f.mAux0 = 0;
    }
}


/***************************************************************************************************
  Column effects (WLED "1.5D": one virtual strip per matrix column)
 **************************************************************************************************/
/* Tetrix - by Blaz Kristan */
struct tTetris { float mPos; float mSpeed; uint8_t mCol; uint16_t mBrick; uint16_t mStack; uint32_t mStep; };

static void FxTetrix(Fx& f)
{
    const int wLen = f.Len();
    tTetris* wDrops = reinterpret_cast<tTetris*>(f.mData);

    for (int wS = 0; wS < f.Strips(); wS++)
    {
        tTetris* wDrop = &wDrops[wS];
        if (f.mCall == 0)
        {
            wDrop->mStack = 0;
            wDrop->mStep  = f.mNow + 2000;
            if (f.mCheck1) wDrop->mCol = 0;
        }

        if (wDrop->mStep == 0)
        {
            int wSpeed = f.mSpeed ? f.mSpeed : random8(1, 255);
            wSpeed = map(wSpeed, 1, 255, 5000, 250);
            wDrop->mSpeed = float(wLen * FRAMETIME) / float(wSpeed);
            wDrop->mPos   = wLen;
            if (!f.mCheck1) wDrop->mCol = random8(0, 15) << 4;
            wDrop->mStep  = 1;
            wDrop->mBrick = (f.mIntensity ? (f.mIntensity >> 5) + 1 : random8(1, 5)) * (1 + (wLen >> 6));
        }

        if (wDrop->mStep == 1)
        {
            if (random8() >> 6) wDrop->mStep = 2;
        }

        if (wDrop->mStep == 2)
        {
            if (wDrop->mPos > wDrop->mStack)
            {
                wDrop->mPos -= wDrop->mSpeed;
                if (int(wDrop->mPos) < int(wDrop->mStack)) wDrop->mPos = wDrop->mStack;
                for (int wI = int(wDrop->mPos); wI < wLen; wI++)
                {
                    const CRGB wCol = (wI < int(wDrop->mPos) + wDrop->mBrick) ? f.ColorFromPal(wDrop->mCol, false, false, 0) : f.mColors[1];
                    f.SetPixel(Fx::VStrip(wI, wS), wCol);
                }
            }
            else
            {
                wDrop->mStep   = 0;
                wDrop->mStack += wDrop->mBrick;
                if (wDrop->mStack >= wLen) wDrop->mStep = f.mNow + 2000;
            }
        }

        if (wDrop->mStep > 2)
        {
            wDrop->mBrick = 0;
            if (wDrop->mStep > f.mNow)
            {
                for (int wI = 0; wI < wLen; wI++) f.BlendPixel(Fx::VStrip(wI, wS), f.mColors[1], 25);
            }
            else
            {
                wDrop->mStack = 0;
                wDrop->mStep  = 0;
                if (f.mCheck1) wDrop->mCol += 8;
            }
        }
    }
}

/* Fire 2012 - by Mark Kriegsman */
static void FxFire2012(Fx& f)
{
    const int wLen = f.Len();
    uint8_t* wHeatAll = f.mData;
    if (f.mCall == 0) f.Fill(CRGB::Black);

    const uint32_t wIt = f.mNow >> 5;

    for (int wS = 0; wS < f.Strips(); wS++)
    {
        uint8_t* wHeat = &wHeatAll[wS * wLen];
        const uint8_t wIgnition = std::max(3, wLen / 10);

        for (int wI = 0; wI < wLen; wI++)
        {
            const uint8_t wCool    = (wIt != f.mStep) ? random8((((20 + f.mSpeed / 3) * 16) / wLen) + 2) : random8(4);
            const uint8_t wMinTemp = (wI < wIgnition) ? (wIgnition - wI) / 4 + 16 : 0;
            const uint8_t wTemp    = qsub8(wHeat[wI], wCool);
            wHeat[wI] = std::max(wMinTemp, wTemp);
        }

        if (wIt != f.mStep)
        {
            for (int wK = wLen - 1; wK > 1; wK--) wHeat[wK] = (wHeat[wK - 1] + (wHeat[wK - 2] << 1)) / 3;

            if (random8() <= f.mIntensity)
            {
                const uint8_t wY = random8(wIgnition);
                const uint8_t wBoost = (17 + f.mCustom3) * (wIgnition - wY / 2) / wIgnition;
                wHeat[wY] = qadd8(wHeat[wY], random8(96 + 2 * wBoost, 207 + wBoost));
            }
        }

        for (int wJ = 0; wJ < wLen; wJ++)
        {
            f.SetPixel(Fx::VStrip(wJ, wS), ColorFromPalette(f.mPalette, std::min<uint8_t>(wHeat[wJ], 240), 255, NOBLEND));
        }
    }

    uint8_t wBlur = f.mCustom2 >> 2;
    if (wBlur > 48) wBlur += wBlur - 48;
    if (wBlur < 16) f.BlurCols(f.mCustom2 >> 1);
    else            f.Blur(wBlur);

    if (wIt != f.mStep) f.mStep = wIt;
}

/* Bouncing Balls */
struct tBall { uint32_t mLastBounceTime; float mImpactVelocity; float mHeight; };

static void FxBouncingBalls(Fx& f)
{
    const int wLen = f.Len();
    constexpr int wMaxBalls = 16;
    tBall* wBallsAll = reinterpret_cast<tBall*>(f.mData);

    if (!f.mCheck2) f.Fill(f.mColors[2] ? CRGB(CRGB::Black) : f.mColors[1]);

    const int   wNumBalls = (f.mIntensity * (wMaxBalls - 1)) / 255 + 1;
    const float wGravity  = -9.81f;
    const bool  wHasCol2  = static_cast<bool>(f.mColors[2]);

    for (int wS = 0; wS < f.Strips(); wS++)
    {
        tBall* wBalls = &wBallsAll[wS * wMaxBalls];
        if (f.mCall == 0) for (int wI = 0; wI < wMaxBalls; wI++) wBalls[wI].mLastBounceTime = f.mNow;

        for (int wI = 0; wI < wNumBalls; wI++)
        {
            const float wSince  = (f.mNow - wBalls[wI].mLastBounceTime) / float((255 - f.mSpeed) / 64 + 1);
            const float wSec    = wSince / 1000.0f;
            wBalls[wI].mHeight  = (0.5f * wGravity * wSec + wBalls[wI].mImpactVelocity) * wSec;

            if (wBalls[wI].mHeight <= 0.0f)
            {
                wBalls[wI].mHeight = 0.0f;
                const float wDampening = 0.9f - float(wI) / float(wNumBalls * wNumBalls);
                wBalls[wI].mImpactVelocity  = wDampening * wBalls[wI].mImpactVelocity;
                wBalls[wI].mLastBounceTime  = f.mNow;
                if (wBalls[wI].mImpactVelocity < 0.015f)
                {
                    wBalls[wI].mImpactVelocity = sqrtf(-2.0f * wGravity) * random8(5, 11) / 10.0f;
                }
            }
            else if (wBalls[wI].mHeight > 1.0f)
            {
                continue;
            }

            CRGB wColor = f.mColors[0];
            if (f.mPaletteIndex) wColor = f.Wheel(wI * (256 / std::max(wNumBalls, 8)));
            else if (wHasCol2)   wColor = f.mColors[wI % 3];

            const int wPos = roundf(wBalls[wI].mHeight * (wLen - 1));
            f.SetPixel(Fx::VStrip(wPos, wS), wColor);
        }
    }
}

/* Popcorn - modified from Keith Lord */
static constexpr int mcMaxPopcorn = 21;

static void FxPopcorn(Fx& f)
{
    const int wLen = f.Len();
    int wNeeded = mcMaxPopcorn;
    if (f.Strips() > 8) wNeeded = constrain((f.mIntensity * mcMaxPopcorn) / 255, 2, mcMaxPopcorn);
    wNeeded = std::min<int>(wNeeded, mcDataSize / (sizeof(tSpark) * f.Strips()));
    tSpark* wPopcornAll = reinterpret_cast<tSpark*>(f.mData);

    if (f.mCall == 0) { f.Fill(CRGB::Black); f.mStep = f.mNow; }

    const bool wHasCol2 = static_cast<bool>(f.mColors[2]);
    if (!f.mCheck2) f.Fill(wHasCol2 ? CRGB(CRGB::Black) : f.mColors[1]);

    const uint32_t wMillis = std::min<uint32_t>(std::max<uint32_t>(1U, f.mNow - f.mStep), 200U);
    f.mStep = f.mNow;
    const float wDt = float(wMillis) / 16.0f;

    float wGravity = -0.0001f - (f.mSpeed / 180000.0f);
    wGravity *= std::min(std::max(1, wLen - 1), 255);
    int wNumPopcorn = f.mIntensity * mcMaxPopcorn / 255;
    if (wNumPopcorn == 0) wNumPopcorn = 1;
    wNumPopcorn = std::min(wNumPopcorn, wNeeded);

    for (int wS = 0; wS < f.Strips(); wS++)
    {
        tSpark* wPop = &wPopcornAll[wS * wNeeded];
        for (int wI = 0; wI < wNumPopcorn; wI++)
        {
            if (wPop[wI].mPos >= 0.0f)
            {
                wPop[wI].mPos += wPop[wI].mVel * wDt;
                wPop[wI].mVel += wGravity * wDt;
            }
            else if (random8() < 2)
            {
                wPop[wI].mPos = 0.01f;
                uint16_t wPeak = 128 + random8(128);
                wPeak = (wPeak * (wLen - 1)) >> 8;
                wPop[wI].mVel = sqrtf(-2.01f * wGravity * wPeak);
                if (f.mPaletteIndex)
                {
                    wPop[wI].mColIndex = random8();
                }
                else
                {
                    uint8_t wCol = random8(0, 3);
                    if (!wHasCol2 || !f.mColors[wCol]) wCol = 0;
                    wPop[wI].mColIndex = wCol;
                }
            }

            if (wPop[wI].mPos >= 0.0f)
            {
                CRGB wCol = f.Wheel(wPop[wI].mColIndex);
                if (!f.mPaletteIndex && (wPop[wI].mColIndex < 3)) wCol = f.mColors[wPop[wI].mColIndex];
                const int wLed = wPop[wI].mPos;
                if (wLed < wLen) f.SetPixel(Fx::VStrip(wLed, wS), wCol);
                for (int wN = 1; wN < 4; wN++)
                {
                    CRGB wTrail = wCol;
                    wTrail.nscale8_video(128 - 32 * wN);
                    if ((wPop[wI].mVel < -wN) && (wLed + wN < wLen)) f.SetPixel(Fx::VStrip(wLed + wN, wS), wTrail);
                    if ((wPop[wI].mVel >  wN) && (wLed >= wN))      f.SetPixel(Fx::VStrip(wLed - wN, wS), wTrail);
                }
            }
        }
    }
}

/* Drip */
struct __attribute__((packed)) tSparkDrop { float mPos; float mBoost; float mVel; uint32_t mAux; uint16_t mCol; uint8_t mColIndex; };

static void FxDrip(Fx& f)
{
    const int wLen = f.Len();
    constexpr int wMaxDrops = 4;
    tSparkDrop* wDropsAll = reinterpret_cast<tSparkDrop*>(f.mData);

    if (f.mCall == 0) { f.Fill(CRGB::Black); f.mStep = f.mNow; }
    if (!f.mCheck2) f.Fill(f.mColors[1]);

    const uint32_t wMillis = std::min<uint32_t>(std::max<uint32_t>(1U, f.mNow - f.mStep), 180U);
    f.mStep = f.mNow;
    const float wDt = float(wMillis) / 20.0f;

    const uint8_t wNumDrops = 1 + (f.mIntensity >> 6);
    const float   wSpeed    = (f.mSpeed * f.mSpeed) / 255.0f;
    float wGravity = -0.0002f - wSpeed / 42000.0f;
    wGravity *= std::min(std::max(1, wLen - 1), 255);
    constexpr int wSourceDrop = 12;

    for (int wS = 0; wS < f.Strips(); wS++)
    {
        tSparkDrop* wDrops = &wDropsAll[wS * wMaxDrops];
        for (int wJ = 0; wJ < wNumDrops; wJ++)
        {
            tSparkDrop& wD = wDrops[wJ];
            if (wD.mColIndex == 0)
            {
                wD.mPos = wLen - 1;
                wD.mVel = 0;
                wD.mCol = wSourceDrop;
                wD.mColIndex = 1;
                const CRGB wC = f.ColorFromPal(1 + random8(254), false, PALETTE_SOLID_WRAP, 0);
                wD.mAux = (uint32_t(wC.r) << 16) | (uint32_t(wC.g) << 8) | wC.b;
                wD.mBoost = 0.65f + float(random8()) / 512.0f;
            }

            const CRGB wDropColor(wD.mAux);
            f.SetPixel(Fx::VStrip(wLen - 1, wS), Blend(CRGB(CRGB::Black), wDropColor, wSourceDrop));

            if (wD.mColIndex == 1)
            {
                if (wD.mCol > 255) wD.mCol = 255;
                const int wIntPos = std::max(0.0f, roundf(wD.mPos));
                f.SetPixel(Fx::VStrip(wIntPos, wS), Blend(CRGB(CRGB::Black), wDropColor, wD.mCol));

                const unsigned wSwell = map(f.mCustom1, 0, 255, 1, 6);
                wD.mCol += wSwell;
                if (wD.mBoost < 4.0f) wD.mBoost += 0.012f * float(wSwell);
                const uint32_t wFallRate = (wD.mCol * (1 + f.mCustom1 * f.mCustom1)) / 192;
                if (random16() <= (wFallRate / 10))
                {
                    wD.mColIndex = 2;
                    wD.mCol = 255;
                    wD.mVel = wGravity * 2.0f * wD.mBoost;
                }
            }

            if (wD.mColIndex > 1)
            {
                if (wD.mPos > 0.01f)
                {
                    wD.mPos += wD.mVel * wDt;
                    if (wD.mPos < 0) wD.mPos = 0;
                    wD.mVel += wGravity * wDt;

                    const int wMaxLen = 8 + f.mSpeed / 64;
                    for (int wI = 1; wI < wMaxLen - wD.mColIndex; wI++)
                    {
                        const int wIntPos = roundf(wD.mPos + float(wI));
                        if ((wIntPos >= wLen) || (wIntPos < 0)) break;
                        f.SetPixel(Fx::VStrip(wIntPos, wS), Blend(CRGB(CRGB::Black), wDropColor, wD.mCol / wI));
                    }
                    if (wD.mColIndex > 2) f.AddPixel(Fx::VStrip(0, wS), Blend(wDropColor, CRGB(CRGB::Black), wD.mCol * 4));
                }
                else
                {
                    if (wD.mColIndex > 2)
                    {
                        wD.mColIndex = 0;
                    }
                    else
                    {
                        if (wD.mColIndex == 2)
                        {
                            wD.mVel = (wLen > 16) ? (-wD.mVel / 3.5f) : (-wD.mVel / 4.5f);
                            wD.mPos += wD.mVel * wDt * wD.mBoost * 0.5f;
                        }
                        wD.mCol = wSourceDrop * 2;
                        wD.mColIndex = 5;
                    }
                }
            }
        }
    }
}


/***************************************************************************************************
  More 1D effects
 **************************************************************************************************/
static void FxFireFlicker(Fx& f)
{
    const uint32_t wCycle = 40 + (255 - f.mSpeed);
    const uint32_t wIt    = f.mNow / wCycle;
    if (f.mStep == wIt) return;

    const CRGB wC = f.mColors[0];
    uint8_t wLum = (f.mPaletteIndex == 0) ? std::max(wC.r, std::max(wC.g, wC.b)) : 255;
    wLum /= (((256 - f.mIntensity) / 16) + 1);
    for (int wI = 0; wI < f.Len(); wI++)
    {
        const uint8_t wFlicker = random8(wLum);
        if (f.mPaletteIndex == 0) f.SetPixel(wI, CRGB(qsub8(wC.r, wFlicker), qsub8(wC.g, wFlicker), qsub8(wC.b, wFlicker)));
        else                      f.SetPixel(wI, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0, 255 - wFlicker));
    }
    f.mStep = wIt;
}

static void GradientBase(Fx& f, bool aLoading)
{
    const int wLen = f.Len();
    const uint16_t wCounter = f.mNow * ((f.mSpeed >> 2) + 1);
    int wPp = (uint32_t(wCounter) * wLen) >> 16;
    if (f.mCall == 0) wPp = 0;
    float wBrd = aLoading ? f.mIntensity : f.mIntensity / 2;
    if (wBrd < 1.0f) wBrd = 1.0f;
    const int wP1 = wPp - wLen;
    const int wP2 = wPp + wLen;

    for (int wI = 0; wI < wLen; wI++)
    {
        float wVal;
        if (aLoading) wVal = abs(((wI > wPp) ? wP2 : wPp) - wI);
        else          wVal = std::min(abs(wPp - wI), std::min(abs(wP1 - wI), abs(wP2 - wI)));
        wVal = (wBrd > wVal) ? (wVal / wBrd * 255) : 255;
        f.SetPixel(wI, blend(f.mColors[0], f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 1), uint8_t(wVal)));
    }
}

static void FxGradient(Fx& f) { GradientBase(f, false); }
static void FxLoading(Fx& f)  { GradientBase(f, true); }

/* Rolling Balls - by pjhatch */
struct tRollingBall { uint32_t mLastUpdate; float mMass; float mVelocity; float mHeight; };

static void FxRollingBalls(Fx& f)
{
    const int wLen = f.Len();
    constexpr int wMaxBalls = 16;
    tRollingBall* wBalls = reinterpret_cast<tRollingBall*>(f.mData);
    const int wNumBalls = f.mIntensity / 16 + 1;

    if (f.mCall == 0)
    {
        for (int wI = 0; wI < wMaxBalls; wI++)
        {
            wBalls[wI].mLastUpdate = f.mNow;
            wBalls[wI].mVelocity = 20.0f * float(random16(1000, 10000)) / 10000.0f;
            if (random8() < 128) wBalls[wI].mVelocity = -wBalls[wI].mVelocity;
            wBalls[wI].mHeight = float(random16(0, 10000)) / 10000.0f;
            wBalls[wI].mMass   = float(random16(1000, 10000)) / 10000.0f;
        }
    }

    const float wCfac = float(scale8(8, 255 - f.mSpeed) + 1) * 20000.0f;
    const bool  wHasCol2 = static_cast<bool>(f.mColors[2]);
    if (!f.mCheck2) f.FadeOut(253);

    for (int wI = 0; wI < wNumBalls; wI++)
    {
        float wHeightNow = wBalls[wI].mHeight + wBalls[wI].mVelocity * (float(f.mNow - wBalls[wI].mLastUpdate) / wCfac);
        if ((wHeightNow < -0.5f) || (wHeightNow > 1.5f))
        {
            wHeightNow = wBalls[wI].mHeight = float(random16(0, 10000)) / 10000.0f;
            wBalls[wI].mLastUpdate = f.mNow;
        }
        if (((wHeightNow <= 0.0f) && (wBalls[wI].mVelocity < 0.0f)) || ((wHeightNow >= 1.0f) && (wBalls[wI].mVelocity > 0.0f)))
        {
            wBalls[wI].mVelocity   = -wBalls[wI].mVelocity;
            wBalls[wI].mLastUpdate = f.mNow;
            wBalls[wI].mHeight     = wHeightNow;
        }

        if (f.mCheck1)
        {
            for (int wJ = wI + 1; wJ < wNumBalls; wJ++)
            {
                if (wBalls[wJ].mVelocity == wBalls[wI].mVelocity) continue;
                const float wTc = (wCfac * (wBalls[wI].mHeight - wBalls[wJ].mHeight) +
                                  wBalls[wI].mVelocity * float(wBalls[wJ].mLastUpdate - wBalls[wI].mLastUpdate)) /
                                  (wBalls[wJ].mVelocity - wBalls[wI].mVelocity);
                if ((wTc > 2.0f) && (wTc < float(f.mNow - wBalls[wJ].mLastUpdate)))
                {
                    wBalls[wI].mHeight = wBalls[wI].mHeight + wBalls[wI].mVelocity * (wTc + float(wBalls[wJ].mLastUpdate - wBalls[wI].mLastUpdate)) / wCfac;
                    wBalls[wJ].mHeight = wBalls[wI].mHeight;
                    wBalls[wI].mLastUpdate = uint32_t(wTc + 0.5f) + wBalls[wJ].mLastUpdate;
                    wBalls[wJ].mLastUpdate = wBalls[wI].mLastUpdate;
                    const float wVtmp = wBalls[wI].mVelocity;
                    const float wMi = wBalls[wI].mMass, wMj = wBalls[wJ].mMass;
                    wBalls[wI].mVelocity = ((wMi - wMj) * wVtmp + 2.0f * wMj * wBalls[wJ].mVelocity) / (wMi + wMj);
                    wBalls[wJ].mVelocity = ((wMj - wMi) * wBalls[wJ].mVelocity + 2.0f * wMi * wVtmp) / (wMi + wMj);
                    wHeightNow = wBalls[wI].mHeight + wBalls[wI].mVelocity * (f.mNow - wBalls[wI].mLastUpdate) / wCfac;
                }
            }
        }

        CRGB wColor = f.mColors[0];
        if (f.mPaletteIndex) wColor = f.ColorFromPal(wI * 255 / wNumBalls, false, PALETTE_SOLID_WRAP, 0);
        else if (wHasCol2)   wColor = f.mColors[wI % 3];

        wHeightNow = constrain(wHeightNow, 0.0f, 1.0f);
        f.SetPixel(roundf(wHeightNow * (wLen - 1)), wColor);
        wBalls[wI].mLastUpdate = f.mNow;
        wBalls[wI].mHeight     = wHeightNow;
    }
}

/* Fairy - inspired by https://www.youtube.com/watch?v=zeOw5MZWq24 */
struct tFlasher { uint16_t mStateStart; uint8_t mStateDur; bool mStateOn; };

static void FxFairy(Fx& f)
{
    const int wLen = f.Len();
    uint16_t wPrng = 5100;
    for (int wI = 0; wI < wLen; wI++)
    {
        wPrng = uint16_t(wPrng * 2053) + 1384;
        f.SetPixel(wI, f.ColorFromPal(wPrng >> 8, false, false, 0));
    }
    if (f.mIntensity == 0) return;

    const uint8_t  wDistance   = ((255 - f.mIntensity) / 28) + 1;
    const uint16_t wNumFlashers = (wLen / wDistance) + 1;
    tFlasher* wFlashers = reinterpret_cast<tFlasher*>(f.mData);
    const uint16_t wNow16 = f.mNow & 0xFFFF;

    uint16_t wZones = wNumFlashers / 6;
    if (!wZones) wZones = 1;
    uint16_t wInZone = wNumFlashers / wZones;
    uint8_t  wFlasherBri[32];

    for (int wZ = 0; wZ < wZones; wZ++)
    {
        uint16_t wSum = 0;
        const uint16_t wFirst = wZ * wInZone;
        if (wZ == wZones - 1) wInZone = wNumFlashers - (wInZone * (wZones - 1));
        wInZone = std::min<uint16_t>(wInZone, 32);

        for (int wF = wFirst; wF < wFirst + wInZone; wF++)
        {
            tFlasher& wFl = wFlashers[wF];
            uint16_t wStateTime = wNow16 - wFl.mStateStart;
            if (wStateTime > wFl.mStateDur * 10)
            {
                wFl.mStateOn = !wFl.mStateOn;
                if (wFl.mStateOn) wFl.mStateDur = 12 + random8(12 + ((255 - f.mSpeed) >> 2));
                else              wFl.mStateDur = 20 + random8(6 + ((255 - f.mSpeed) >> 2));
                wFl.mStateStart = wNow16;
                if (wStateTime < 255)
                {
                    wFl.mStateStart -= 255 - wStateTime;
                    wFl.mStateDur   += 26 - wStateTime / 10;
                    wStateTime = 255 - wStateTime;
                }
                else
                {
                    wStateTime = 0;
                }
            }
            if (wStateTime > 255) wStateTime = 255;
            wFlasherBri[wF - wFirst] = wFl.mStateOn ? wStateTime : 255 - wStateTime;
            wSum += wFlasherBri[wF - wFirst];
        }

        const uint8_t wAvg  = wSum / wInZone;
        const uint8_t wPeak = 255 - ((wAvg * 92) >> 8);

        for (int wF = wFirst; wF < wFirst + wInZone; wF++)
        {
            const uint8_t wBri = (wFlasherBri[wF - wFirst] * wPeak) / 255;
            wPrng = uint16_t(wPrng * 2053) + 1384;
            const int wPos = wF * wDistance;
            f.SetPixel(wPos, blend(f.mColors[1], f.ColorFromPal(wPrng >> 8, false, false, 0), wBri));
            for (int wI = wPos + 1; (wI < wPos + wDistance) && (wI < wLen); wI++)
            {
                wPrng = uint16_t(wPrng * 2053) + 1384;
                f.SetPixel(wI, f.ColorFromPal(wPrng >> 8, false, false, 0, wPeak));
            }
        }
    }
}

static void FxTwoDots(Fx& f)
{
    const int wLen = f.Len();
    if (!f.mCheck2) f.Fill(f.mColors[2]);
    const CRGB wColor2 = (f.mColors[1] == f.mColors[2]) ? f.mColors[0] : f.mColors[1];

    const uint16_t wDelay  = 1 + (FRAMETIME << 3) / wLen;
    const uint32_t wIt     = f.mNow / map(f.mSpeed, 0, 255, wDelay << 4, wDelay);
    const int      wOffset = wIt % wLen;
    int wWidth = (wLen * (f.mIntensity + 1)) >> 9;
    if (!wWidth) wWidth = 1;
    for (int wI = 0; wI < wWidth; wI++)
    {
        f.SetPixel((wOffset + wI) % wLen, f.mColors[0]);
        f.SetPixel((wOffset + wI + (wLen >> 1)) % wLen, wColor2);
    }
}

static void FxFairytwinkle(Fx& f)
{
    const int wLen = f.Len();
    tFlasher* wFlashers = reinterpret_cast<tFlasher*>(f.mData);
    const uint16_t wNow16 = f.mNow & 0xFFFF;
    uint16_t wPrng = 5100;

    const uint16_t wRiseFall = 400 + (255 - f.mSpeed) * 3;
    const uint16_t wMaxDur   = wRiseFall / 100 + ((255 - f.mIntensity) >> 2) + 13 + ((255 - f.mIntensity) >> 1);

    for (int wF = 0; wF < wLen; wF++)
    {
        tFlasher& wFl = wFlashers[wF];
        uint16_t wStateTime = wNow16 - wFl.mStateStart;
        if (wStateTime > wFl.mStateDur * 100)
        {
            wFl.mStateOn = !wFl.mStateOn;
            const bool wInit = !wFl.mStateDur;
            if (wFl.mStateOn) wFl.mStateDur = wRiseFall / 100 + ((255 - f.mIntensity) >> 2) + random8(12 + ((255 - f.mIntensity) >> 1)) + 1;
            else              wFl.mStateDur = wRiseFall / 100 + random8(3 + ((255 - f.mSpeed) >> 6)) + 1;
            wFl.mStateStart = wNow16;
            wStateTime = 0;
            if (wInit)
            {
                wFl.mStateStart -= wRiseFall;
                wFl.mStateDur = wRiseFall / 100 + random8(12 + ((255 - f.mIntensity) >> 1)) + 5;
                wStateTime = wRiseFall;
            }
        }
        if (wFl.mStateOn && (wFl.mStateDur > wMaxDur)) wFl.mStateDur = wMaxDur;
        if (wStateTime > wRiseFall) wStateTime = wRiseFall;

        const uint8_t wFadeProg = 255 - ((wStateTime * 255) / wRiseFall);
        const uint8_t wBri = wFl.mStateOn ? 255 - dim8_video(wFadeProg) : dim8_video(wFadeProg);

        const uint16_t wLast = wPrng;
        uint16_t wDiff = 0;
        while (wDiff < 0x4000)
        {
            wPrng = uint16_t(wPrng * 2053) + 1384;
            wDiff = (wPrng > wLast) ? wPrng - wLast : wLast - wPrng;
        }
        f.SetPixel(wF, blend(f.mColors[1], f.ColorFromPal(wPrng >> 8, false, false, 0), wBri));
    }
}

static void FxChase3(Fx& f)
{
    const uint32_t wCycle = 50 + ((255 - f.mSpeed) << 1);
    const uint32_t wIt    = f.mNow / wCycle;
    const uint8_t  wWidth = 1 + (f.mIntensity >> 4);
    uint8_t wIndex = wIt % (wWidth * 3);

    for (int wI = 0; wI < f.Len(); wI++, wIndex++)
    {
        if (wIndex > (wWidth * 3) - 1) wIndex = 0;
        CRGB wColor = f.mColors[2];
        if (wIndex > (wWidth << 1) - 1) wColor = f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 1);
        else if (wIndex > wWidth - 1)   wColor = f.mColors[0];
        f.SetPixel(f.Len() - wI - 1, wColor);
    }
}

static void FxTriWipe(Fx& f)
{
    const int wLen = f.Len();
    const uint32_t wCycle = 1000 + (255 - f.mSpeed) * 200;
    const uint32_t wPerc  = f.mNow % wCycle;
    const uint16_t wProg  = (wPerc * 65535) / wCycle;
    const int wLedIndex   = (uint32_t(wProg) * wLen * 3) >> 16;

    for (int wI = 0; wI < wLen; wI++) f.SetPixel(wI, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 2));

    if (wLedIndex < wLen)
    {
        for (int wI = 0; wI < wLen; wI++) f.SetPixel(wI, (wI > wLedIndex) ? f.mColors[0] : f.mColors[1]);
    }
    else if (wLedIndex < wLen * 2)
    {
        for (int wI = wLedIndex - wLen + 1; wI < wLen; wI++) f.SetPixel(wI, f.mColors[1]);
    }
    else
    {
        for (int wI = 0; wI <= wLedIndex - wLen * 2; wI++) f.SetPixel(wI, f.mColors[0]);
    }
}

static void FxTriFade(Fx& f)
{
    const uint16_t wCounter = f.mNow * ((f.mSpeed >> 3) + 1);
    const uint32_t wProg    = (uint32_t(wCounter) * 768) >> 16;

    CRGB wColor1, wColor2;
    uint8_t wStage;
    if (wProg < 256)      { wColor1 = f.mColors[0]; wColor2 = f.mColors[1]; wStage = 0; }
    else if (wProg < 512) { wColor1 = f.mColors[1]; wColor2 = f.mColors[2]; wStage = 1; }
    else                  { wColor1 = f.mColors[2]; wColor2 = f.mColors[0]; wStage = 2; }

    const uint8_t wStp = wProg;
    for (int wI = 0; wI < f.Len(); wI++)
    {
        CRGB wColor;
        if (wStage == 2)      wColor = blend(f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 2), wColor2, wStp);
        else if (wStage == 1) wColor = blend(wColor1, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 2), wStp);
        else                  wColor = blend(wColor1, wColor2, wStp);
        f.SetPixel(wI, wColor);
    }
}

static void FxLightning(Fx& f)
{
    const int wLen = f.Len();
    const int wStart = random16(wLen);
    const int wLenF  = 1 + random16(wLen - wStart);
    uint8_t wBri = 255 / random8(1, 3);

    if (f.mAux1 == 0)
    {
        f.mAux1 = random8(4, 4 + f.mIntensity / 20);
        f.mAux1 *= 2;
        wBri = 52;
        f.mAux0 = 200;
    }

    if (!f.mCheck2) f.Fill(f.mColors[1]);

    if ((f.mAux1 > 3) && !(f.mAux1 & 0x01))
    {
        for (int wI = wStart; wI < wStart + wLenF; wI++) f.SetPixel(wI, f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0, wBri));
        f.mAux1--;
        f.mStep = f.mNow;
    }
    else if ((f.mNow - f.mStep) > f.mAux0)
    {
        f.mAux1--;
        if (f.mAux1 < 2) f.mAux1 = 0;
        f.mAux0 = 50 + random8(100);
        if (f.mAux1 == 2) f.mAux0 = random8(255 - f.mSpeed) * 100;
        f.mStep = f.mNow;
    }
}

static void FxIcu(Fx& f)
{
    const int wLen = f.Len();
    int wDest = f.mStep & 0xFFFF;
    const uint8_t wSpace = (f.mIntensity >> 3) + 2;

    if (!f.mCheck2) f.Fill(f.mColors[1]);

    const uint8_t wPIndex = map(wDest, 0, wLen - wLen / wSpace, 0, 255);
    const CRGB    wCol    = f.ColorFromPal(wPIndex, false, false, 0);

    f.SetPixel(wDest, wCol);
    f.SetPixel(wDest + wLen / wSpace, wCol);

    if (f.mAux0 == wDest)
    {
        if (random8(6) == 0)
        {
            f.SetPixel(wDest, f.mColors[1]);
            f.SetPixel(wDest + wLen / wSpace, f.mColors[1]);
            f.mDelay = 200;
            return;
        }
        f.mAux0 = random16(wLen - wLen / wSpace);
        f.mDelay = 1000 + random16(2000);
        return;
    }

    if (f.mAux0 > f.mStep)      { f.mStep++; wDest++; }
    else if (f.mAux0 < f.mStep) { f.mStep--; wDest--; }

    f.SetPixel(wDest, wCol);
    f.SetPixel(wDest + wLen / wSpace, wCol);
    f.mDelay = SpeedFormulaL(f);
}

static void FxMultiComet(Fx& f)
{
    const uint32_t wCycle = 10 + uint32_t(255 - f.mSpeed);
    const uint32_t wIt    = f.mNow / wCycle;
    if (f.mStep == wIt) return;

    uint16_t* wComets = reinterpret_cast<uint16_t*>(f.mData);
    if (f.mCall == 0) f.Fill(CRGB::Black);
    f.FadeOut(f.mIntensity);

    for (int wI = 0; wI < 8; wI++)
    {
        if (wComets[wI] < f.Len())
        {
            const int wIndex = wComets[wI];
            if (f.mColors[2]) f.SetPixel(wIndex, (wI % 2) ? f.ColorFromPal(wIndex, true, PALETTE_SOLID_WRAP, 0) : f.mColors[2]);
            else              f.SetPixel(wIndex, f.ColorFromPal(wIndex, true, PALETTE_SOLID_WRAP, 0));
            wComets[wI]++;
        }
        else if (!random16(f.Len()))
        {
            wComets[wI] = 0;
        }
    }
    f.mStep = wIt;
}

static void FxStream2(Fx& f)
{
    if (f.mCall == 0)
    {
        f.mStep = (uint32_t(random8()) << 16) | (uint32_t(random8()) << 8) | random8();
        f.mAux0 = random16();
    }
    const uint16_t wPrevSeed = random16_get_seed();
    const uint32_t wCycle = 25 + (3 * uint32_t(255 - f.mSpeed));
    const uint32_t wIt    = f.mNow / wCycle;
    uint32_t wColor = f.mStep;
    random16_set_seed(f.mAux0);

    for (int wI = f.Len() - 1; wI >= 0; wI--)
    {
        const uint8_t wR = random8(6) != 0 ? ((wColor >> 16) & 0xFF) : random8();
        const uint8_t wG = random8(6) != 0 ? ((wColor >> 8) & 0xFF)  : random8();
        const uint8_t wB = random8(6) != 0 ? (wColor & 0xFF)         : random8();
        wColor = (uint32_t(wR) << 16) | (uint32_t(wG) << 8) | wB;
        f.SetPixel(wI, CRGB(wR, wG, wB));
        if ((wI == f.Len() - 1) && (f.mAux1 != (wIt & 0xFFFF)))
        {
            f.mStep = wColor;
            f.mAux0 = random16_get_seed();
        }
    }
    f.mAux1 = wIt & 0xFFFF;
    random16_set_seed(wPrevSeed);
}

struct tOscillator { int16_t mPos; int8_t mSize; int8_t mDir; int8_t mSpeed; };

static void FxOscillate(Fx& f)
{
    const int wLen = f.Len();
    tOscillator* wOsc = reinterpret_cast<tOscillator*>(f.mData);

    if (f.mCall == 0)
    {
        wOsc[0] = { int16_t(wLen / 4),     int8_t(wLen / 8),  1, 1 };
        wOsc[1] = { int16_t(wLen / 4 * 3), int8_t(wLen / 8),  1, 2 };
        wOsc[2] = { int16_t(wLen / 4 * 2), int8_t(wLen / 8), -1, 1 };
    }

    const uint32_t wCycle = 20 + (2 * uint32_t(255 - f.mSpeed));
    const uint32_t wIt    = f.mNow / wCycle;

    for (int wI = 0; wI < 3; wI++)
    {
        if (wIt != f.mStep) wOsc[wI].mPos += wOsc[wI].mDir * wOsc[wI].mSpeed;
        wOsc[wI].mSize = wLen / (3 + f.mIntensity / 8);
        if ((wOsc[wI].mDir == -1) && (wOsc[wI].mPos <= 0))
        {
            wOsc[wI].mPos = 0;
            wOsc[wI].mDir = 1;
            wOsc[wI].mSpeed = (f.mSpeed > 100) ? random8(2, 4) : random8(1, 3);
        }
        if ((wOsc[wI].mDir == 1) && (wOsc[wI].mPos >= (wLen - 1)))
        {
            wOsc[wI].mPos = wLen - 1;
            wOsc[wI].mDir = -1;
            wOsc[wI].mSpeed = (f.mSpeed > 100) ? random8(2, 4) : random8(1, 3);
        }
    }

    for (int wI = 0; wI < wLen; wI++)
    {
        CRGB wColor = CRGB::Black;
        for (int wJ = 0; wJ < 3; wJ++)
        {
            if ((wI >= wOsc[wJ].mPos - wOsc[wJ].mSize) && (wI <= wOsc[wJ].mPos + wOsc[wJ].mSize))
            {
                wColor = (wColor == CRGB(CRGB::Black)) ? f.mColors[wJ] : blend(wColor, f.mColors[wJ], 128);
            }
        }
        f.SetPixel(wI, wColor);
    }
    f.mStep = wIt;
}

/* Pride 2015 - by Mark Kriegsman */
static void FxPride2015(Fx& f)
{
    const uint16_t wDuration = 10 + f.mSpeed;
    uint16_t wPseudotime = f.mStep;
    uint16_t wHue16Start = f.mAux0;

    const uint8_t  wSat8      = beatsin88(87, 220, 250);
    const uint8_t  wBrightDepth = beatsin88(341, 96, 224);
    const uint16_t wBrightInc = beatsin88(203, (25 * 256), (40 * 256));
    const uint8_t  wMsMult    = beatsin88(147, 23, 60);

    uint16_t wHue16 = wHue16Start;
    const uint16_t wHueInc16 = beatsin88(113, 1, 3000);

    if (f.mCall == 0) f.Fill(CRGB::Black);

    wPseudotime += wDuration * wMsMult;
    wHue16Start += wDuration * beatsin88(400, 5, 9);
    uint16_t wTheta = wPseudotime;

    for (int wI = 0; wI < f.Len(); wI++)
    {
        wHue16 += wHueInc16;
        const uint8_t wHue8 = wHue16 >> 8;
        wTheta += wBrightInc;
        const uint16_t wB16  = sin16(wTheta) + 32768;
        const uint16_t wBri16 = (uint32_t(wB16) * uint32_t(wB16)) / 65536;
        uint8_t wBri8 = (uint32_t(wBri16) * wBrightDepth) / 65536;
        wBri8 += (255 - wBrightDepth);
        f.BlendPixel(wI, CHSV(wHue8, wSat8, wBri8), 64);
    }
    f.mStep = wPseudotime;
    f.mAux0 = wHue16Start;
}

static void FxJuggle(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);
    f.FadeAll(192 - (3 * f.mIntensity / 4));

    uint8_t wDotHue = 0;
    for (int wI = 0; wI < 8; wI++)
    {
        const int wIndex = beatsin88((16 + f.mSpeed) * (wI + 7), 0, f.Len() - 1);
        CRGB wC = f.GetPixel(wIndex);
        wC |= (f.mPaletteIndex == 0) ? CRGB(CHSV(wDotHue, 220, 255)) : ColorFromPalette(f.mPalette, wDotHue, 255);
        f.SetPixel(wIndex, wC);
        wDotHue += 32;
    }
}

static void FxPalette(Fx& f)
{
    uint16_t wCounter = 0;
    if (f.mSpeed != 0)
    {
        wCounter = (f.mNow * ((f.mSpeed >> 3) + 1)) & 0xFFFF;
        wCounter >>= 8;
    }
    for (int wI = 0; wI < f.Len(); wI++)
    {
        const uint8_t wIndex = (wI * 255 / f.Len()) - wCounter;
        f.SetPixel(wI, f.ColorFromPal(wIndex, false, PaletteMovingWrap(f), 255));
    }
}

/* ColorWavesWithPalettes - by Mark Kriegsman */
static void FxColorwaves(Fx& f)
{
    const uint16_t wDuration = 10 + f.mSpeed;
    uint16_t wPseudotime = f.mStep;
    uint16_t wHue16Start = f.mAux0;

    const uint8_t  wBrightDepth = beatsin88(341, 96, 224);
    const uint16_t wBrightInc   = beatsin88(203, (25 * 256), (40 * 256));
    const uint8_t  wMsMult      = beatsin88(147, 23, 60);

    uint16_t wHue16 = wHue16Start;
    const uint16_t wHueInc16 = beatsin88(113, 60, 300) * f.mIntensity * 10 / 255;

    wPseudotime += wDuration * wMsMult;
    wHue16Start += wDuration * beatsin88(400, 5, 9);
    uint16_t wTheta = wPseudotime;

    if (f.mCall == 0) f.Fill(CRGB::Black);

    for (int wI = 0; wI < f.Len(); wI++)
    {
        wHue16 += wHueInc16;
        uint8_t wHue8 = wHue16 >> 8;
        const uint16_t wH128 = wHue16 >> 7;
        if (wH128 & 0x100) wHue8 = 255 - (wH128 >> 1);
        else               wHue8 = wH128 >> 1;

        wTheta += wBrightInc;
        const uint16_t wB16  = sin16(wTheta) + 32768;
        const uint16_t wBri16 = (uint32_t(wB16) * uint32_t(wB16)) / 65536;
        uint8_t wBri8 = (uint32_t(wBri16) * wBrightDepth) / 65536;
        wBri8 += (255 - wBrightDepth);
        f.BlendPixel(wI, f.ColorFromPal(wHue8, false, PALETTE_SOLID_WRAP, 0, wBri8), 128);
    }
    f.mStep = wPseudotime;
    f.mAux0 = wHue16Start;
}

static void FxBpm(Fx& f)
{
    const uint32_t wStp  = (f.mNow / 20) & 0xFF;
    const uint8_t  wBeat = beatsin8(f.mSpeed, 64, 255);
    for (int wI = 0; wI < f.Len(); wI++)
    {
        f.SetPixel(wI, f.ColorFromPal(wStp + (wI * 2), false, PALETTE_SOLID_WRAP, 0, wBeat - wStp + (wI * 10)));
    }
}

static void FxFillNoise(Fx& f)
{
    if (f.mCall == 0) f.mStep = random16(12345);
    for (int wI = 0; wI < f.Len(); wI++)
    {
        const uint8_t wIndex = inoise8(wI * f.Len(), f.mStep + wI * f.Len());
        f.SetPixel(wI, f.ColorFromPal(wIndex, false, PALETTE_SOLID_WRAP, 0));
    }
    f.mStep += beatsin8(f.mSpeed, 1, 6);
}

static void FxNoise1(Fx& f)
{
    constexpr uint16_t wScale = 320;
    f.mStep += (1 + f.mSpeed / 16);
    for (int wI = 0; wI < f.Len(); wI++)
    {
        const uint16_t wShiftX = beatsin8(11);
        const uint16_t wShiftY = f.mStep / 42;
        const uint16_t wRealX  = (wI + wShiftX) * wScale;
        const uint16_t wRealY  = (wI + wShiftY) * wScale;
        const uint8_t  wNoise  = inoise16(uint32_t(wRealX), uint32_t(wRealY), f.mStep) >> 8;
        f.SetPixel(wI, f.ColorFromPal(sin8(wNoise * 3), false, PALETTE_SOLID_WRAP, 0));
    }
}

static void FxNoise2(Fx& f)
{
    constexpr uint16_t wScale = 1000;
    f.mStep += (1 + (f.mSpeed >> 1));
    for (int wI = 0; wI < f.Len(); wI++)
    {
        const uint16_t wShiftX = f.mStep >> 6;
        const uint32_t wRealX  = (wI + wShiftX) * wScale;
        const uint8_t  wNoise  = inoise16(wRealX, 0, 4223) >> 8;
        f.SetPixel(wI, f.ColorFromPal(sin8(wNoise * 3), false, PALETTE_SOLID_WRAP, 0, wNoise));
    }
}

static void FxNoise3(Fx& f)
{
    constexpr uint16_t wScale = 800;
    f.mStep += (1 + f.mSpeed);
    for (int wI = 0; wI < f.Len(); wI++)
    {
        const uint32_t wRealX = (wI + 4223) * wScale;
        const uint32_t wRealY = (wI + 1234) * wScale;
        const uint32_t wRealZ = f.mStep * 8;
        const uint8_t  wNoise = inoise16(wRealX, wRealY, wRealZ) >> 8;
        f.SetPixel(wI, f.ColorFromPal(sin8(wNoise * 3), false, PALETTE_SOLID_WRAP, 0, wNoise));
    }
}

static void FxNoise4(Fx& f)
{
    const uint32_t wStp = (f.mNow * f.mSpeed) >> 7;
    for (int wI = 0; wI < f.Len(); wI++)
    {
        const int16_t wIndex = inoise16(uint32_t(wI) << 12, wStp);
        f.SetPixel(wI, f.ColorFromPal(uint8_t(wIndex), false, PALETTE_SOLID_WRAP, 0));
    }
}

/** @brief Advances the WLED Colortwinkles envelope for one pixel or word. */
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

static void FxColortwinkles(Fx& f)
{
    uint8_t* wBits = f.mData;
    if (f.mCall == 0) f.Fill(CRGB::Black);

    for (int wI = 0; wI < f.Len(); wI++)
    {
        CRGB wC = f.GetPixel(wI);
        bool wFadeUp = bitRead(wBits[wI >> 3], wI & 0x07);
        StepColorTwinkle(wC, wFadeUp, f.mSpeed);
        bitWrite(wBits[wI >> 3], wI & 0x07, wFadeUp);
        f.SetPixel(wI, wC);
    }

    for (int wJ = 0; wJ <= f.Len() / 50; wJ++)
    {
        if (random8() > f.mIntensity) continue;
        for (int wTimes = 0; wTimes < 5; wTimes++)
        {
            const int wI = random16(f.Len());
            if (f.GetPixel(wI) == CRGB(CRGB::Black))
            {
                bitWrite(wBits[wI >> 3], wI & 0x07, true);
                f.SetPixel(wI, ColorFromPalette(f.mPalette, random8(), 64, NOBLEND));
                break;
            }
        }
    }
}

static void FxLake(Fx& f)
{
    const uint8_t wSp    = f.mSpeed / 10;
    const int     wWave1 = BeatSin8Signed64(wSp + 2);
    const int     wWave2 = BeatSin8Signed64(wSp + 1);
    const uint8_t wWave3 = beatsin8(wSp + 2, 0, 80);

    for (int wI = 0; wI < f.Len(); wI++)
    {
        const int     wIndex = cos8((wI * 15) + wWave1) / 2 + cubicwave8((wI * 23) + wWave2) / 2;
        const uint8_t wLum   = (wIndex > wWave3) ? wIndex - wWave3 : 0;
        f.SetPixel(wI, f.ColorFromPal(wIndex, false, false, 0, wLum));
    }
}

/* Meteor & Meteor Smooth - merged by dedehai */
static void MeteorCore(Fx& f, bool aSmooth)
{
    const int wLen = f.Len();
    uint8_t* wTrail = f.mData;
    const bool wSmooth = aSmooth || f.mCheck3;
    const int  wSize   = 1 + wLen / 20;

    int wStart;
    if (wSmooth)
    {
        wStart = map((f.mStep >> 6) & 0xFF, 0, 255, 0, wLen - 1);
    }
    else
    {
        const uint16_t wCounter = f.mNow * ((f.mSpeed >> 2) + 8);
        wStart = (uint32_t(wCounter) * wLen) >> 16;
    }

    const int wMax = !f.mCheck1 ? 240 : 255;
    for (int wI = 0; wI < wLen; wI++)
    {
        if (random8() > 255 - f.mIntensity) continue;
        CRGB wCol;
        if (wSmooth)
        {
            if (wTrail[wI] > 0) wTrail[wI] = constrain(wTrail[wI] + 4 - random8(24), 0, wMax);
            wCol = f.mCheck1 ? f.ColorFromPal(wI, true, false, 0, wTrail[wI]) : f.ColorFromPal(wTrail[wI], false, true, 255);
        }
        else
        {
            wTrail[wI] = scale8(wTrail[wI], 128 + random8(127));
            int wIndex = wTrail[wI];
            uint8_t wSlot = 255;
            if (!f.mCheck1) { wSlot = 0; wIndex = map(wI, 0, wLen, 0, wMax); }
            wCol = f.ColorFromPal(wIndex, false, false, wSlot, wTrail[wI]);
        }
        f.SetPixel(wI, wCol);
    }

    for (int wJ = 0; wJ < wSize; wJ++)
    {
        const int wIndex = (wStart + wJ) % wLen;
        if (wSmooth)
        {
            wTrail[wIndex] = wMax;
            f.SetPixel(wIndex, f.mCheck1 ? f.ColorFromPal(wIndex, true, false, 0, wTrail[wIndex]) : f.ColorFromPal(wTrail[wIndex], false, true, 255));
        }
        else
        {
            uint8_t wSlot = 255;
            int wI = wTrail[wIndex] = wMax;
            if (!f.mCheck1) { wI = map(wIndex, 0, wLen, 0, wMax); wSlot = 0; }
            f.SetPixel(wIndex, f.ColorFromPal(wI, false, false, wSlot, 255));
        }
    }
    f.mStep += f.mSpeed + 1;
}

static void FxMeteor(Fx& f)       { MeteorCore(f, false); }
static void FxMeteorSmooth(Fx& f) { MeteorCore(f, true); }

static void FxRailway(Fx& f)
{
    const uint16_t wDur  = (256 - f.mSpeed) * 40;
    const uint16_t wRamp = (uint32_t(wDur) * f.mIntensity) >> 8;
    if (f.mStep > wDur) { f.mStep = 0; f.mAux0 = !f.mAux0; }

    uint8_t wPos = 255;
    if (wRamp != 0)
    {
        const uint16_t wP0 = (f.mStep * 255) / wRamp;
        if (wP0 < 255) wPos = wP0;
    }
    if (f.mAux0) wPos = 255 - wPos;

    for (int wI = 0; wI < f.Len(); wI += 2)
    {
        f.SetPixel(wI, f.ColorFromPal(255 - wPos, false, false, 255));
        if (wI < f.Len() - 1) f.SetPixel(wI + 1, f.ColorFromPal(wPos, false, false, 255));
    }
    f.mStep += FRAMETIME;
}

/* TwinkleFOX - by Mark Kriegsman */
static CRGB TwinkleOne(Fx& f, uint32_t aMs, uint8_t aSalt, bool aCat)
{
    const uint16_t wTicks = aMs / f.mAux0;
    const uint8_t  wFast8 = wTicks;
    uint16_t wSlow16 = (wTicks >> 8) + aSalt;
    wSlow16 += sin8(wSlow16);
    wSlow16 = (wSlow16 * 2053) + 1384;
    const uint8_t wSlow8 = (wSlow16 & 0xFF) + (wSlow16 >> 8);

    const uint8_t wDensity = (f.mIntensity >> 5) + 1;
    uint8_t wBright = 0;
    if (((wSlow8 & 0x0E) / 2) < wDensity)
    {
        uint8_t wPh = wFast8;
        if (aCat)
        {
            wBright = 255 - wPh;
            if (f.mCheck2) wBright = wPh;
        }
        else if (wPh < 86)
        {
            wBright = wPh * 3;
        }
        else
        {
            wPh -= 86;
            wBright = 255 - (wPh + (wPh / 2));
        }
    }

    const uint8_t wHue = wSlow8 - aSalt;
    CRGB wC = CRGB::Black;
    if (wBright > 0)
    {
        wC = ColorFromPalette(f.mPalette, wHue, wBright, NOBLEND);
        if (!f.mCheck1 && (wFast8 >= 128))
        {
            const uint8_t wCooling = (wFast8 - 128) >> 4;
            wC.g = qsub8(wC.g, wCooling);
            wC.b = qsub8(wC.b, wCooling * 2);
        }
    }
    return wC;
}

static void TwinkleFoxBase(Fx& f, bool aCat)
{
    uint16_t wPrng = 11337;
    if (f.mSpeed > 100) f.mAux0 = 3 + ((255 - f.mSpeed) >> 3);
    else                f.mAux0 = 22 + ((100 - f.mSpeed) >> 1);

    CRGB wBg = f.mColors[1];
    const uint8_t wBgLight = wBg.getAverageLight();
    if (wBgLight > 64)      wBg.nscale8_video(16);
    else if (wBgLight > 16) wBg.nscale8_video(64);
    else                    wBg.nscale8_video(86);
    const uint8_t wBgBright = wBg.getAverageLight();

    for (int wI = 0; wI < f.Len(); wI++)
    {
        wPrng = uint16_t(wPrng * 2053) + 1384;
        const uint16_t wClockOffset = wPrng;
        wPrng = uint16_t(wPrng * 2053) + 1384;
        const uint8_t  wSpeedMult = ((((wPrng & 0xFF) >> 4) + (wPrng & 0x0F)) & 0x0F) + 0x08;
        const uint32_t wClock = uint32_t((f.mNow * wSpeedMult) >> 3) + wClockOffset;
        const uint8_t  wUnique = wPrng >> 8;

        const CRGB wC = TwinkleOne(f, wClock, wUnique, aCat);
        const int16_t wDelta = wC.getAverageLight() - wBgBright;
        if ((wDelta >= 32) || !wBg) f.SetPixel(wI, wC);
        else if (wDelta > 0)        f.SetPixel(wI, Blend(wBg, wC, wDelta * 8));
        else                        f.SetPixel(wI, wBg);
    }
}

static void FxTwinklefox(Fx& f) { TwinkleFoxBase(f, false); }
static void FxTwinklecat(Fx& f) { TwinkleFoxBase(f, true); }

static void FxSolidPattern(Fx& f)
{
    const uint16_t wLit = 1 + f.mSpeed, wUnlit = 1 + f.mIntensity;
    bool wDrawingLit = true;
    uint16_t wCnt = 0;
    for (int wI = 0; wI < f.Len(); wI++)
    {
        f.SetPixel(wI, wDrawingLit ? f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0) : f.mColors[1]);
        wCnt++;
        if (wCnt >= (wDrawingLit ? wLit : wUnlit)) { wCnt = 0; wDrawingLit = !wDrawingLit; }
    }
}

static void FxSolidPatternTri(Fx& f)
{
    const uint8_t wSegSize = (f.mIntensity >> 5) + 1;
    uint8_t  wCurr = 0;
    uint16_t wCount = 0;
    for (int wI = 0; wI < f.Len(); wI++)
    {
        if ((wCurr % 3) == 0)      f.SetPixel(wI, f.mColors[0]);
        else if ((wCurr % 3) == 1) f.SetPixel(wI, f.mColors[1]);
        else                       f.SetPixel(wI, f.mColors[2] ? f.mColors[2] : CRGB(CRGB::White));
        wCount++;
        if (wCount >= wSegSize) { wCurr++; wCount = 0; }
    }
}

static void SpotsBase(Fx& f, uint16_t aThreshold)
{
    const int wLen = f.Len();
    if (!f.mCheck2) f.Fill(f.mColors[1]);

    const unsigned wMaxZones = std::max(1, wLen / 4);
    const int      wZones    = 1U + ((uint32_t(f.mIntensity) * wMaxZones + 127) >> 8);
    const int      wZoneLen  = (wLen + wZones - 1) / wZones;
    const int      wOffset   = (wLen - (wZones * wZoneLen)) / 2;

    for (int wZ = 0; wZ < wZones; wZ++)
    {
        const int wPos = wOffset + (wZ * wZoneLen);
        for (int wI = 0; wI < wZoneLen; wI++)
        {
            const unsigned wWave = TriWave16((wI * 0xFFFF) / wZoneLen);
            if (wWave > aThreshold)
            {
                const int wIndex = wPos + wI;
                const unsigned wS = ((wWave - aThreshold) * 255 / (0xFFFF - aThreshold)) & 0xFF;
                f.SetPixel(wIndex, blend(f.ColorFromPal(wIndex, true, PALETTE_SOLID_WRAP, 0), f.mColors[1], uint8_t(255 - wS)));
            }
        }
    }
}

static void FxSpots(Fx& f)
{
    SpotsBase(f, (255 - f.mSpeed) << 8);
}

static void FxSpotsFade(Fx& f)
{
    const uint16_t wCounter = f.mNow * ((f.mSpeed >> 2) + 8);
    const uint16_t wT  = TriWave16(wCounter);
    const uint16_t wTr = (wT >> 1) + (wT >> 2);
    SpotsBase(f, wTr);
}

static void GlitterBase(Fx& f, uint8_t aIntensity, const CRGB& arColor)
{
    if (aIntensity > random8()) f.SetPixel(random16(f.Len()), arColor);
}

static void FxGlitter(Fx& f)
{
    if (!f.mCheck2) FxPalette(f);
    GlitterBase(f, f.mIntensity, f.mColors[2] ? f.mColors[2] : CRGB(CRGB::White));
}

static void FxSolidGlitter(Fx& f)
{
    f.Fill(f.mColors[0]);
    GlitterBase(f, f.mIntensity, f.mColors[2] ? f.mColors[2] : CRGB(CRGB::White));
}

static void CandleBase(Fx& f, bool aMulti)
{
    const uint8_t wRange  = f.mIntensity;
    const uint8_t wRndVal = wRange >> 1;

    uint8_t wSpeedFactor = 4;
    if (f.mSpeed > 252)     wSpeedFactor = 1;
    else if (f.mSpeed > 99) wSpeedFactor = 2;
    else if (f.mSpeed > 49) wSpeedFactor = 3;

    const int wNum = aMulti ? f.Len() : 1;
    for (int wI = 0; wI < wNum; wI++)
    {
        int wD = 0;
        uint8_t wS = f.mAux0, wTarget = f.mAux1, wFadeStep = f.mStep;
        if (wI > 0)
        {
            wD = (wI - 1) * 3;
            wS = f.mData[wD]; wTarget = f.mData[wD + 1]; wFadeStep = f.mData[wD + 2];
        }
        if (wFadeStep == 0) { wS = 128; wTarget = 130 + random8(4); wFadeStep = 1; }

        bool wNewTarget = false;
        if (wTarget > wS) { wS = qadd8(wS, wFadeStep); if (wS >= wTarget) wNewTarget = true; }
        else              { wS = qsub8(wS, wFadeStep); if (wS <= wTarget) wNewTarget = true; }

        if (wNewTarget)
        {
            wTarget = random8(wRndVal) + random8(wRndVal);
            if (wTarget < (wRndVal >> 1)) wTarget = (wRndVal >> 1) + random8(wRndVal);
            wTarget += 255 - wRange;
            const uint8_t wDif = (wTarget > wS) ? wTarget - wS : wS - wTarget;
            wFadeStep = wDif >> wSpeedFactor;
            if (wFadeStep == 0) wFadeStep = 1;
        }

        if (wI > 0)
        {
            f.SetPixel(wI, blend(f.mColors[1], f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0), wS));
            f.mData[wD] = wS; f.mData[wD + 1] = wTarget; f.mData[wD + 2] = wFadeStep;
        }
        else
        {
            for (int wJ = 0; wJ < f.Len(); wJ++) f.SetPixel(wJ, blend(f.mColors[1], f.ColorFromPal(wJ, true, PALETTE_SOLID_WRAP, 0), wS));
            f.mAux0 = wS; f.mAux1 = wTarget; f.mStep = wFadeStep;
        }
    }
}

static void FxCandle(Fx& f)      { CandleBase(f, false); }
static void FxCandleMulti(Fx& f) { CandleBase(f, true); }

/* Fireworks Starburst */
static constexpr int mcStarburstFragments = 10;
struct tStar
{
    CRGB     mColor;
    uint32_t mBirth;
    uint32_t mLast;
    float    mVel;
    uint16_t mPos;
    float    mFragment[mcStarburstFragments];
};

static void FxStarburst(Fx& f)
{
    const int wLen = f.Len();
    const unsigned wMaxStars = mcDataSize / sizeof(tStar);
    const unsigned wNumStars = std::min<unsigned>(1 + (wLen >> 3), wMaxStars);
    tStar* wStars = reinterpret_cast<tStar*>(f.mData);

    constexpr float wMaxSpeed = 375.0f, wIgnition = 250.0f, wFadeTime = 1500.0f;

    for (unsigned wJ = 0; wJ < wNumStars; wJ++)
    {
        if ((random8(144 - (f.mSpeed >> 1)) == 0) && (wStars[wJ].mBirth == 0))
        {
            const uint16_t wStart = (wLen > 1) ? random16(wLen - 1) : 0;
            const float wMultiplier = float(random8()) / 255.0f;
            wStars[wJ].mColor = f.Wheel(random8());
            wStars[wJ].mPos   = wStart;
            wStars[wJ].mVel   = wMaxSpeed * float(random8()) / 255.0f * wMultiplier;
            wStars[wJ].mBirth = f.mNow;
            wStars[wJ].mLast  = f.mNow;
            const int wNum = random8(3, 6 + (f.mIntensity >> 5));
            for (int wI = 0; wI < mcStarburstFragments; wI++) wStars[wJ].mFragment[wI] = (wI < wNum) ? wStart : -1;
        }
    }

    if (!f.mCheck2) f.Fill(f.mColors[1]);

    for (unsigned wJ = 0; wJ < wNumStars; wJ++)
    {
        tStar& wS = wStars[wJ];
        if (wS.mBirth != 0)
        {
            const float wDt = (f.mNow - wS.mLast) / 1000.0f;
            for (int wI = 0; wI < mcStarburstFragments; wI++)
            {
                const int wVar = wI >> 1;
                if (wS.mFragment[wI] > 0) wS.mFragment[wI] += wS.mVel * wDt * float(wVar) / 3.0f;
            }
            wS.mLast = f.mNow;
            wS.mVel -= 3 * wS.mVel * wDt;
        }

        CRGB  wC = wS.mColor;
        float wFade = 0.0f;
        float wAge  = f.mNow - wS.mBirth;
        if (wAge < wIgnition)
        {
            wC = Blend(CRGB(CRGB::White), wC, int(254.5f * (wAge / wIgnition)));
        }
        else if (wAge > wIgnition + wFadeTime)
        {
            wFade = 1.0f;
            wS.mBirth = 0;
            wC = f.mColors[1];
        }
        else
        {
            wAge -= wIgnition;
            wFade = wAge / wFadeTime;
            wC = Blend(wC, f.mColors[1], int(254.5f * wFade));
        }

        const float wSize = (1.0f - wFade) * 2.0f;
        for (int wIndex = 0; wIndex < mcStarburstFragments * 2; wIndex++)
        {
            const bool wMirrored = wIndex & 0x1;
            const int  wI = wIndex >> 1;
            if (wS.mFragment[wI] <= 0) continue;
            float wLoc = wS.mFragment[wI];
            if (wMirrored) wLoc -= (wLoc - wS.mPos) * 2;
            int wStart = wLoc - wSize;
            int wEnd   = wLoc + wSize;
            if (wStart < 0) wStart = 0;
            if (wStart == wEnd) wEnd++;
            if (wEnd > wLen) wEnd = wLen;
            for (int wP = wStart; wP < wEnd; wP++) f.SetPixel(wP, wC);
        }
    }
}

static void SinelonBase(Fx& f, bool aDual, bool aRainbow)
{
    const int wLen = f.Len();
    if (f.mCall == 0) f.Fill(CRGB::Black);
    f.FadeOut(f.mIntensity);

    const int wPos = beatsin16(f.mSpeed / 10, 0, wLen - 1);
    if (f.mCall == 0) f.mAux0 = wPos;

    CRGB wColor1 = f.ColorFromPal(wPos, true, false, 0);
    CRGB wColor2 = f.mColors[2];
    if (aRainbow) wColor1 = f.Wheel((wPos & 0x07) * 32);
    f.SetPixel(wPos, wColor1);
    if (aDual)
    {
        if (!wColor2) wColor2 = f.ColorFromPal(wPos, true, false, 0);
        if (aRainbow) wColor2 = wColor1;
        f.SetPixel(wLen - 1 - wPos, wColor2);
    }
    if (f.mAux0 != wPos)
    {
        if (f.mAux0 < wPos)
        {
            for (int wI = f.mAux0; wI < wPos; wI++) { f.SetPixel(wI, wColor1); if (aDual) f.SetPixel(wLen - 1 - wI, wColor2); }
        }
        else
        {
            for (int wI = f.mAux0; wI > wPos; wI--) { f.SetPixel(wI, wColor1); if (aDual) f.SetPixel(wLen - 1 - wI, wColor2); }
        }
        f.mAux0 = wPos;
    }
}

static void FxSinelon(Fx& f)        { SinelonBase(f, false, false); }
static void FxSinelonDual(Fx& f)    { SinelonBase(f, true, false); }
static void FxSinelonRainbow(Fx& f) { SinelonBase(f, false, true); }

static void FxPlasma(Fx& f)
{
    if (f.mCall == 0) f.mAux0 = random8(0, 2);
    const uint8_t wThisPhase = BeatSin8Signed64(6 + f.mAux0);
    const uint8_t wThatPhase = BeatSin8Signed64(7 + f.mAux0);

    for (int wI = 0; wI < f.Len(); wI++)
    {
        const uint8_t wColorIndex = cubicwave8((wI * (2 + 3 * (f.mSpeed >> 5)) + wThisPhase) & 0xFF) / 2
                                  + cos8((wI * (1 + 2 * (f.mSpeed >> 5)) + wThatPhase) & 0xFF) / 2;
        const uint8_t wBright = qsub8(wColorIndex, beatsin8(7, 0, (128 - (f.mIntensity >> 1))));
        f.SetPixel(wI, f.ColorFromPal(wColorIndex, false, PALETTE_SOLID_WRAP, 0, wBright));
    }
}

static void FxPercent(Fx& f)
{
    const int wLen = f.Len();
    const uint8_t wPercent = std::min<uint8_t>(f.mIntensity, 200);
    const uint16_t wActive = (wPercent < 100) ? (wLen * wPercent / 100.0f) : (wLen * (200 - wPercent) / 100.0f);

    uint8_t wSize = 1 + ((f.mSpeed * wLen) >> 11);
    if (f.mSpeed == 255) wSize = 255;

    if (wPercent <= 100)
    {
        for (int wI = 0; wI < wLen; wI++)
        {
            if (wI < f.mAux1) f.SetPixel(wI, f.mCheck1 ? f.ColorFromPal(map(wPercent, 0, 100, 0, 255), false, false, 0) : f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0));
            else              f.SetPixel(wI, f.mColors[1]);
        }
    }
    else
    {
        for (int wI = 0; wI < wLen; wI++)
        {
            if (wI < (wLen - f.mAux1)) f.SetPixel(wI, f.mColors[1]);
            else f.SetPixel(wI, f.mCheck1 ? f.ColorFromPal(map(wPercent, 100, 200, 255, 0), false, false, 0) : f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0));
        }
    }

    if (wActive > f.mAux1)
    {
        f.mAux1 += wSize;
        if (f.mAux1 > wActive) f.mAux1 = wActive;
    }
    else if (wActive < f.mAux1)
    {
        f.mAux1 = (f.mAux1 > wSize) ? (f.mAux1 - wSize) : 0;
        if (f.mAux1 < wActive) f.mAux1 = wActive;
    }
}

static void FxHeartbeat(Fx& f)
{
    const uint8_t  wBpm        = 40 + (f.mSpeed >> 3);
    const uint32_t wMsPerBeat  = 60000L / wBpm;
    const uint32_t wSecondBeat = wMsPerBeat / 3;
    uint32_t wBriLower = f.mAux1;
    const uint32_t wBeatTimer = f.mNow - f.mStep;

    wBriLower = wBriLower * 2042 / (2048 + f.mIntensity);
    f.mAux1 = wBriLower;

    if ((wBeatTimer > wSecondBeat) && !f.mAux0) { f.mAux1 = 0xFFFF; f.mAux0 = 1; }
    if (wBeatTimer > wMsPerBeat)                { f.mAux1 = 0xFFFF; f.mAux0 = 0; f.mStep = f.mNow; }

    for (int wI = 0; wI < f.Len(); wI++)
    {
        f.SetPixel(wI, blend(f.ColorFromPal(wI, true, PALETTE_SOLID_WRAP, 0), f.mColors[1], 255 - (f.mAux1 >> 8)));
    }
}

/* Pacifica - by Mark Kriegsman and Mary Corey March */
static CRGB PacificaOneLayer(Fx& f, uint16_t aI, const CRGBPalette16& arPal, uint16_t aCiStart, uint16_t aWaveScale, uint8_t aBri, uint16_t aIOff)
{
    uint16_t wCi = aCiStart;
    uint16_t wWaveAngle = aIOff;
    const uint16_t wHalf = (aWaveScale >> 1) + 20;

    wWaveAngle += ((120 + f.mIntensity) * aI);
    const uint16_t wS16 = sin16(wWaveAngle) + 32768;
    const uint16_t wCs  = scale16(wS16, wHalf) + wHalf;
    wCi += (wCs * aI);
    const uint16_t wSIndex16 = sin16(wCi) + 32768;
    return ColorFromPalette(arPal, scale16(wSIndex16, 240), aBri, LINEARBLEND);
}

static void FxPacifica(Fx& f)
{
    static const CRGBPalette16 wcPal1(
        CRGB(0x000507), CRGB(0x000409), CRGB(0x00030B), CRGB(0x00030D), CRGB(0x000210), CRGB(0x000212), CRGB(0x000114), CRGB(0x000117),
        CRGB(0x000019), CRGB(0x00001C), CRGB(0x000026), CRGB(0x000031), CRGB(0x00003B), CRGB(0x000046), CRGB(0x14554B), CRGB(0x28AA50));
    static const CRGBPalette16 wcPal2(
        CRGB(0x000507), CRGB(0x000409), CRGB(0x00030B), CRGB(0x00030D), CRGB(0x000210), CRGB(0x000212), CRGB(0x000114), CRGB(0x000117),
        CRGB(0x000019), CRGB(0x00001C), CRGB(0x000026), CRGB(0x000031), CRGB(0x00003B), CRGB(0x000046), CRGB(0x0C5F52), CRGB(0x19BE5F));
    static const CRGBPalette16 wcPal3(
        CRGB(0x000208), CRGB(0x00030E), CRGB(0x000514), CRGB(0x00061A), CRGB(0x000820), CRGB(0x000927), CRGB(0x000B2D), CRGB(0x000C33),
        CRGB(0x000E39), CRGB(0x001040), CRGB(0x001450), CRGB(0x001860), CRGB(0x001C70), CRGB(0x002080), CRGB(0x1040BF), CRGB(0x2060FF));

    const CRGBPalette16& wP1 = f.mPaletteIndex ? f.mPalette : wcPal1;
    const CRGBPalette16& wP2 = f.mPaletteIndex ? f.mPalette : wcPal2;
    const CRGBPalette16& wP3 = f.mPaletteIndex ? f.mPalette : wcPal3;

    uint16_t wS1 = f.mAux0, wS2 = f.mAux1, wS3 = f.mStep & 0xFFFF, wS4 = f.mStep >> 16;
    const uint32_t wDeltaMs = (FRAMETIME >> 2) + ((FRAMETIME * f.mSpeed) >> 7);
    const uint32_t wT = (f.mNow >> 2) + ((uint64_t(f.mNow) * f.mSpeed) >> 7);

    const uint16_t wSf1 = BeatSin16(3, 179, 269, wT);
    const uint16_t wSf2 = BeatSin16(4, 179, 269, wT);
    const uint32_t wD1  = (wDeltaMs * wSf1) / 256;
    const uint32_t wD2  = (wDeltaMs * wSf2) / 256;
    const uint32_t wD21 = (wD1 + wD2) / 2;
    wS1 += (wD1  * BeatSin88(1011, 10, 13, wT));
    wS2 -= (wD21 * BeatSin88(777, 8, 11, wT));
    wS3 -= (wD1  * BeatSin88(501, 5, 7, wT));
    wS4 -= (wD2  * BeatSin88(257, 4, 6, wT));
    f.mAux0 = wS1;
    f.mAux1 = wS2;
    f.mStep = (uint32_t(wS4) << 16) + wS3;

    const uint8_t wBaseThreshold = BeatSin8(9, 55, 65, wT);
    uint8_t wWave = Beat8(7, wT);

    for (int wI = 0; wI < f.Len(); wI++)
    {
        CRGB wC = CRGB(2, 6, 10);
        wC += PacificaOneLayer(f, wI, wP1, wS1, BeatSin16(3, 11 * 256, 14 * 256, wT), BeatSin8(10, 70, 130, wT), 0 - Beat16(301, wT));
        wC += PacificaOneLayer(f, wI, wP2, wS2, BeatSin16(4,  6 * 256,  9 * 256, wT), BeatSin8(17, 40,  80, wT),     Beat16(401, wT));
        wC += PacificaOneLayer(f, wI, wP3, wS3, 6 * 256, BeatSin8(9, 10, 38, wT), 0 - Beat16(503, wT));
        wC += PacificaOneLayer(f, wI, wP3, wS4, 5 * 256, BeatSin8(8, 10, 28, wT),     Beat16(601, wT));

        const uint8_t wThreshold = scale8(sin8(wWave), 20) + wBaseThreshold;
        wWave += 7;
        const uint8_t wL = wC.getAverageLight();
        if (wL > wThreshold)
        {
            const uint8_t wOver  = wL - wThreshold;
            const uint8_t wOver2 = qadd8(wOver, wOver);
            wC += CRGB(wOver, wOver2, qadd8(wOver2, wOver2));
        }
        wC.blue  = scale8(wC.blue, 145);
        wC.green = scale8(wC.green, 200);
        wC |= CRGB(2, 5, 7);
        f.SetPixel(wI, wC);
    }
}

static void FxSunrise(Fx& f)
{
    const int wLen = f.Len();
    if ((f.mCall == 0) || (f.mSpeed != f.mAux0)) { f.mStep = f.mNow; f.mAux0 = f.mSpeed; }

    f.Fill(CRGB::Black);
    uint16_t wStage = 0xFFFF;
    uint32_t wS10 = (f.mNow - f.mStep) / 100;

    if (f.mSpeed > 120)
    {
        const uint16_t wCounter = (f.mNow >> 1) * (((f.mSpeed - 120) >> 1) + 1);
        wStage = TriWave16(wCounter);
    }
    else if (f.mSpeed)
    {
        uint8_t wDurMins = f.mSpeed;
        if (wDurMins > 60) wDurMins -= 60;
        const uint32_t wTarget = wDurMins * 600;
        if (wS10 > wTarget) wS10 = wTarget;
        wStage = map(wS10, 0, wTarget, 0, 0xFFFF);
        if (f.mSpeed > 60) wStage = 0xFFFF - wStage;
    }

    for (int wI = 0; wI <= wLen / 2; wI++)
    {
        uint16_t wWave = TriWave16((uint32_t(wI) * wStage) / wLen);
        wWave = (wWave >> 8) + ((wWave * f.mIntensity) >> 15);
        const CRGB wC = (wWave > 240) ? f.ColorFromPal(240, false, true, 255) : f.ColorFromPal(wWave, false, true, 255);
        f.SetPixel(wI, wC);
        f.SetPixel(wLen - wI - 1, wC);
    }
}

/* Phased & Phased Noise - by Andrew Tuline */
static void PhasedBase(Fx& f, uint8_t aModer)
{
    constexpr uint8_t wAllFreq = 16;
    float wPhase;
    memcpy(&wPhase, &f.mStep, sizeof(wPhase));

    const uint8_t wCutOff = 255 - f.mIntensity;
    uint8_t wModVal = 5;
    uint8_t wIndex  = f.mNow / 64;
    wPhase += f.mSpeed / 32.0f;

    for (int wI = 0; wI < f.Len(); wI++)
    {
        if (aModer == 1) wModVal = inoise8(wI * 10 + wI * 10) / 16;
        uint16_t wVal = (wI + 1) * wAllFreq;
        if (wModVal == 0) wModVal = 1;
        wVal += wPhase * (wI % wModVal + 1) / 2;
        uint8_t wB = cubicwave8(wVal);
        wB = (wB > wCutOff) ? (wB - wCutOff) : 0;
        f.SetPixel(wI, blend(f.mColors[1], f.ColorFromPal(wIndex, false, false, 0), wB));
        wIndex += 256 / f.Len();
        if (f.Len() > 256) wIndex++;
    }
    memcpy(&f.mStep, &wPhase, sizeof(wPhase));
}

static void FxPhased(Fx& f)      { PhasedBase(f, 0); }
static void FxPhasedNoise(Fx& f) { PhasedBase(f, 1); }

static void FxTwinkleup(Fx& f)
{
    const uint16_t wPrevSeed = random16_get_seed();
    random16_set_seed(535);
    for (int wI = 0; wI < f.Len(); wI++)
    {
        const uint8_t wStart = random8();
        uint8_t wBri = sin8(wStart + 16 * f.mNow / (256 - f.mSpeed));
        if (random8() > f.mIntensity) wBri = 0;
        f.SetPixel(wI, blend(f.mColors[1], f.ColorFromPal(random8() + f.mNow / 100, false, PALETTE_SOLID_WRAP, 0), wBri));
    }
    random16_set_seed(wPrevSeed);
}

static void FxNoisePal(Fx& f)
{
    const uint16_t wScale = 15 + (f.mIntensity >> 2);
    CRGBPalette16* wPalettes = reinterpret_cast<CRGBPalette16*>(f.mData);

    const uint32_t wChangeMs = 4000 + f.mSpeed * 10;
    if ((f.mCall == 0) || ((f.mNow - f.mStep) > wChangeMs))
    {
        f.mStep = f.mNow;
        const uint8_t wBase = random8();
        wPalettes[1] = CRGBPalette16(CHSV(wBase + random8(64), 255, random8(128, 255)), CHSV(wBase + 128, 255, random8(128, 255)),
                                     CHSV(wBase + random8(92), 192, random8(128, 255)), CHSV(wBase + random8(92), 255, random8(128, 255)));
    }
    nblendPaletteTowardPalette(wPalettes[0], wPalettes[1], 48);
    if (f.mPaletteIndex > 0) wPalettes[0] = f.mPalette;

    for (int wI = 0; wI < f.Len(); wI++)
    {
        const uint8_t wIndex = inoise8(wI * wScale, f.mAux0 + wI * wScale);
        f.SetPixel(wI, ColorFromPalette(wPalettes[0], wIndex, 255, LINEARBLEND));
    }
    f.mAux0 += beatsin8(10, 1, 4);
}

static void FxSine(Fx& f)
{
    const uint16_t wColorIndex = f.mNow / 32;
    f.mStep += f.mSpeed / 16;
    const uint16_t wFreq = f.mIntensity / 4;
    for (int wI = 0; wI < f.Len(); wI++)
    {
        const int wBri = cubicwave8((wI * wFreq) + f.mStep);
        f.SetPixel(wI, blend(f.mColors[1], f.ColorFromPal(wI * wColorIndex / 255, false, PALETTE_SOLID_WRAP, 0), wBri));
    }
}

static void FxFlow(Fx& f)
{
    const int wLen = f.Len();
    unsigned wCounter = 0;
    if (f.mSpeed != 0) { wCounter = f.mNow * ((f.mSpeed >> 2) + 1); wCounter >>= 8; }

    const unsigned wMaxZones = wLen / 6;
    int wZones = (f.mIntensity * wMaxZones) >> 8;
    if (wZones & 0x01) wZones++;
    if (wZones < 2) wZones = 2;
    const int wZoneLen = wLen / wZones;
    wZones += 2;
    const int wOffset = (wLen - (wZones * wZoneLen)) / 2;

    f.Fill(f.ColorFromPal(-wCounter, false, true, 255));
    for (int wZ = 0; wZ < wZones; wZ++)
    {
        const int wPos = wOffset + wZ * wZoneLen;
        for (int wI = 0; wI < wZoneLen; wI++)
        {
            const uint8_t wColorIndex = (wI * 255 / wZoneLen) - wCounter;
            const int wLed = (wZ & 0x01) ? wI : (wZoneLen - 1) - wI;
            f.SetPixel(wPos + wLed, f.ColorFromPal(wColorIndex, false, true, 255));
        }
    }
}

/* Chunchun - by Aircoookie */
static void FxChunchun(Fx& f)
{
    const int wLen = f.Len();
    if (f.mCall == 0) f.Fill(CRGB::Black);
    f.FadeOut(253);

    uint32_t wCounter = (f.mNow * (96 + f.mSpeed)) >> 4;
    const uint16_t wNumBirds = std::min(32, 2 + (wLen >> 3));
    const uint32_t wSpan = (uint32_t(f.mIntensity) << 8) / wNumBirds;

    for (unsigned wI = 0; wI < wNumBirds; wI++)
    {
        wCounter -= wSpan;
        const uint16_t wMegumin = sin16(wCounter) + 0x8000;
        int wBird = (uint32_t(wMegumin) * wLen) >> 16;
        wBird = std::min(wBird, wLen - 1);
        f.SetPixel(wBird, f.ColorFromPal((wI * 255) / wNumBirds, false, false, 0));
    }
}

/* Dancing Shadows - by Steve Pomeroy */
struct tSpotlight { float mSpeed; uint32_t mLastUpdate; int16_t mPosition; uint8_t mColorIdx; uint8_t mWidth; uint8_t mType; };

static void FxDancingShadows(Fx& f)
{
    const int wLen = f.Len();
    const uint8_t wNum = std::min<int>(map(f.mIntensity, 0, 255, 2, 49), mcDataSize / sizeof(tSpotlight));
    const bool wInitialize = (f.mAux0 != wNum);
    f.mAux0 = wNum;

    tSpotlight* wSpots = reinterpret_cast<tSpotlight*>(f.mData);
    f.Fill(CRGB::Black);

    for (int wI = 0; wI < wNum; wI++)
    {
        tSpotlight& wS = wSpots[wI];
        bool wRespawn = false;
        if (!wInitialize)
        {
            const int16_t wDelta = float(f.mNow - wS.mLastUpdate) * (wS.mSpeed * ((1.0f + f.mSpeed) / 100.0f));
            if (abs(wDelta) >= 1) { wS.mPosition += wDelta; wS.mLastUpdate = f.mNow; }
            wRespawn = ((wS.mSpeed > 0.0f) && (wS.mPosition > (wLen + 2))) || ((wS.mSpeed < 0.0f) && (wS.mPosition < -(wS.mWidth + 2)));
        }

        if (wInitialize || wRespawn)
        {
            wS.mColorIdx = random8();
            wS.mWidth    = random8(1, 10);
            wS.mSpeed    = 1.0f / random8(4, 50);
            if (wInitialize)
            {
                wS.mPosition = random16(wLen);
                wS.mSpeed   *= random8(2) ? 1.0f : -1.0f;
            }
            else if (random8(2))
            {
                wS.mPosition = wLen + wS.mWidth;
                wS.mSpeed   *= -1.0f;
            }
            else
            {
                wS.mPosition = -wS.mWidth;
            }
            wS.mLastUpdate = f.mNow;
            wS.mType = random8(6);
        }

        const CRGB wColor = f.ColorFromPal(wS.mColorIdx, false, false, 255);
        const int  wStart = wS.mPosition;

        if (wS.mWidth <= 1)
        {
            if ((wStart >= 0) && (wStart < wLen)) f.BlendPixel(wStart, wColor, 128);
            continue;
        }

        const int wStepJ = (wS.mType >= 3) ? (wS.mType - 1) : 1;   // 2x/3x/4x dots
        for (int wJ = 0; wJ < wS.mWidth; wJ += wStepJ)
        {
            if ((wStart + wJ) < 0 || (wStart + wJ) >= wLen) continue;
            uint8_t wAmount = 128;
            if (wS.mType == 1) wAmount = cubicwave8(map(wJ, 0, wS.mWidth - 1, 0, 255));
            if (wS.mType == 2) wAmount = cubicwave8(2 * map(wJ, 0, wS.mWidth - 1, 0, 255));
            f.BlendPixel(wStart + wJ, wColor, wAmount);
        }
    }
}

/* Washing Machine - by Stefan Seegel */
static void FxWashingMachine(Fx& f)
{
    const int wSpeed = TristateSquare8(f.mNow >> 7, 90, 15);
    f.mStep += (wSpeed * 2048) / (512 - f.mSpeed);
    for (int wI = 0; wI < f.Len(); wI++)
    {
        const uint8_t wCol = sin8(((f.mIntensity / 25 + 1) * 255 * wI / f.Len()) + (f.mStep >> 7));
        f.SetPixel(wI, f.ColorFromPal(wCol, false, PALETTE_SOLID_WRAP, 3));
    }
}

/* Blends - by Mark Kriegsman */
static void FxBlends(Fx& f)
{
    const int wPixelLen = std::min(f.Len(), 255);
    CRGB* wPixels = reinterpret_cast<CRGB*>(f.mData);
    const uint8_t wBlendSpeed = map(f.mIntensity, 0, 255, 10, 128);
    uint8_t wShift = (f.mNow * ((f.mSpeed >> 3) + 1)) >> 8;

    for (int wI = 0; wI < wPixelLen; wI++)
    {
        wPixels[wI] = blend(wPixels[wI], f.ColorFromPal(wShift + quadwave8((wI + 1) * 16), false, PALETTE_SOLID_WRAP, 255), wBlendSpeed);
        wShift += 3;
    }
    int wOffset = 0;
    for (int wI = 0; wI < f.Len(); wI++)
    {
        f.SetPixel(wI, wPixels[wOffset++]);
        if (wOffset >= wPixelLen) wOffset = 0;
    }
}

/* TV Simulator - by Def3nder, based on "Fake TV Light for Engineers" by Phillip Burgess */
struct tTvSim
{
    uint32_t mTotalTime, mFadeTime, mStartTime, mElapsed;
    uint32_t mSceneStart, mSceneDuration;
    uint16_t mSliderValues, mSceneHue;
    uint8_t  mSceneSat, mSceneBri, mActR, mActG, mActB;
    uint16_t mPr, mPg, mPb;
};

static void FxTvSimulator(Fx& f)
{
    tTvSim* wTv = reinterpret_cast<tTvSim*>(f.mData);
    const uint8_t wColorSpeed     = map(f.mSpeed, 0, 255, 1, 20);
    const uint8_t wColorIntensity = map(f.mIntensity, 0, 255, 10, 30);

    const uint16_t wSliders = (f.mSpeed << 8) | f.mIntensity;
    if (wSliders != wTv->mSliderValues) { wTv->mSliderValues = wSliders; f.mAux1 = 0; }

    if (((f.mNow - wTv->mSceneStart) >= wTv->mSceneDuration) || (f.mAux1 == 0))
    {
        wTv->mSceneStart    = f.mNow;
        wTv->mSceneDuration = random16(uint16_t(60 * 250 * wColorSpeed), uint16_t(60 * 750 * wColorSpeed));
        wTv->mSceneHue      = random16(0, 768);
        wTv->mSceneSat      = random8(100, 130 + wColorIntensity);
        wTv->mSceneBri      = random8(200, 240);
        f.mAux1 = 1;
        f.mAux0 = 0;
    }

    if (f.mAux0 == 0)
    {
        uint8_t wJ = random8(4 * wColorIntensity);
        const uint16_t wH = wTv->mSceneHue;
        const uint16_t wHue = (random8() < 128) ? ((wJ < wH) ? wH - wJ : 767 - wH - wJ) : (((wJ + wH) < 767) ? wH + wJ : wH + wJ - 767);
        wJ = random8(2 * wColorIntensity);
        const uint8_t wSat = (int(wTv->mSceneSat) - wJ < 0) ? 0 : wTv->mSceneSat - wJ;
        wJ = random8(100);
        const uint8_t wBri = (int(wTv->mSceneBri) - wJ < 0) ? 0 : wTv->mSceneBri - wJ;

        uint8_t wTemp[5];
        const uint8_t wN = (wHue >> 8) % 3;
        const uint8_t wX = ((((wHue & 255) * wSat) >> 8) * wBri) >> 8;
        const uint8_t wS = ((256 - wSat) * wBri) >> 8;
        wTemp[0] = wTemp[3] = wS;
        wTemp[1] = wTemp[4] = wX + wS;
        wTemp[2] = wBri - wX;
        wTv->mActR = wTemp[wN + 2];
        wTv->mActG = wTemp[wN + 1];
        wTv->mActB = wTemp[wN];
    }

    const uint16_t wNr = uint16_t(dim8_video(wTv->mActR)) * 257;
    const uint16_t wNg = uint16_t(dim8_video(wTv->mActG)) * 257;
    const uint16_t wNb = uint16_t(dim8_video(wTv->mActB)) * 257;

    if (f.mAux0 == 0)
    {
        f.mAux0 = 1;
        wTv->mTotalTime = random16(250, 2500);
        wTv->mFadeTime  = random16(0, wTv->mTotalTime);
        if (random8(10) < 3) wTv->mFadeTime = 0;
        wTv->mStartTime = f.mNow;
    }

    wTv->mElapsed = f.mNow - wTv->mStartTime;
    uint16_t wR = wNr, wG = wNg, wB = wNb;
    if (wTv->mElapsed < wTv->mFadeTime)
    {
        wR = map(wTv->mElapsed, 0, wTv->mFadeTime, wTv->mPr, wNr);
        wG = map(wTv->mElapsed, 0, wTv->mFadeTime, wTv->mPg, wNg);
        wB = map(wTv->mElapsed, 0, wTv->mFadeTime, wTv->mPb, wNb);
    }
    f.Fill(CRGB(wR >> 8, wG >> 8, wB >> 8));

    if (wTv->mElapsed >= wTv->mTotalTime)
    {
        wTv->mPr = wNr; wTv->mPg = wNg; wTv->mPb = wNb;
        f.mAux0 = 0;
    }
}

/* Aurora */
static constexpr int mcAuroraMaxWaves = 20;
struct tAuroraWave
{
    uint16_t mTtl;
    CRGB     mBaseColor;
    float    mBaseAlpha;
    uint16_t mAge;
    uint16_t mWidth;
    float    mCenter;
    bool     mGoingLeft;
    float    mSpeedFactor;
    bool     mAlive;

    void Init(uint32_t aLen, const CRGB& arColor)
    {
        mTtl       = random16(500, 1501);
        mBaseColor = arColor;
        mBaseAlpha = random8(60, 101) / 100.0f;
        mAge       = 0;
        mWidth     = random16(aLen / 20, aLen / 6);
        if (!mWidth) mWidth = 1;
        mCenter      = random8(101) / 100.0f * aLen;
        mGoingLeft   = random8(0, 2) == 0;
        mSpeedFactor = random8(10, 31) / 100.0f * 6 / 255;
        mAlive       = true;
    }

    CRGB GetColor(int aLed) const
    {
        if ((aLed < mCenter - mWidth) || (aLed > mCenter + mWidth)) return CRGB::Black;
        float wOffset = aLed - mCenter;
        if (wOffset < 0) wOffset = -wOffset;
        const float wOffsetFactor = wOffset / mWidth;
        const float wAgeFactor = (float(mAge) / mTtl < 0.5f) ? float(mAge) / (mTtl / 2) : float(mTtl - mAge) / (float(mTtl) * 0.5f);
        const float wFactor = (1 - wOffsetFactor) * wAgeFactor * mBaseAlpha;
        return CRGB(mBaseColor.r * wFactor, mBaseColor.g * wFactor, mBaseColor.b * wFactor);
    }

    void Update(uint32_t aLen, uint32_t aSpeed)
    {
        mCenter += (mGoingLeft ? -1 : 1) * mSpeedFactor * aSpeed;
        mAge++;
        if (mAge > mTtl)                                  mAlive = false;
        else if (mGoingLeft && (mCenter + mWidth < 0))   mAlive = false;
        else if (!mGoingLeft && (mCenter - mWidth > aLen)) mAlive = false;
    }
};

static void FxAurora(Fx& f)
{
    tAuroraWave* wWaves = reinterpret_cast<tAuroraWave*>(f.mData);

    if ((f.mAux0 != f.mIntensity) || (f.mCall == 0))
    {
        f.mAux1 = map(f.mIntensity, 0, 255, 2, mcAuroraMaxWaves);
        f.mAux0 = f.mIntensity;
        for (int wI = 0; wI < f.mAux1; wI++) wWaves[wI].Init(f.Len(), f.ColorFromPal(random8(), false, false, random8(0, 3)));
    }

    for (int wI = 0; wI < f.mAux1; wI++)
    {
        wWaves[wI].Update(f.Len(), f.mSpeed);
        if (!wWaves[wI].mAlive) wWaves[wI].Init(f.Len(), f.ColorFromPal(random8(), false, false, random8(0, 3)));
    }

    uint8_t wBacklight = 1;
    for (int wI = 0; wI < 3; wI++) if (f.mColors[wI]) wBacklight++;

    for (int wI = 0; wI < f.Len(); wI++)
    {
        CRGB wMixed(wBacklight, wBacklight, wBacklight);
        for (int wJ = 0; wJ < f.mAux1; wJ++) wMixed += wWaves[wJ].GetColor(wI);
        f.SetPixel(wI, wMixed);
    }
}

/* Perlin Move - by Andrew Tuline */
static void FxPerlinMove(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);
    f.FadeOut(255 - f.mCustom1);
    for (int wI = 0; wI < f.mIntensity / 16 + 1; wI++)
    {
        const uint32_t wT = f.mNow * 128 / (260 - f.mSpeed);
        const uint16_t wLocn = inoise16(wT + wI * 15000, wT);
        const int wPix = map(wLocn, 50 * 256, 192 * 256, 0, f.Len() - 1);
        f.SetPixel(wPix, f.ColorFromPal(wPix % 255, false, PALETTE_SOLID_WRAP, 0));
    }
}

/* Wavesins - by Andrew Tuline */
static void FxWavesins(Fx& f)
{
    for (int wI = 0; wI < f.Len(); wI++)
    {
        const uint8_t wBri   = sin8(f.mNow / 4 + wI * f.mIntensity);
        const uint8_t wIndex = beatsin8(f.mSpeed, f.mCustom1, f.mCustom1 + f.mCustom2, 0, wI * (f.mCustom3 << 3));
        f.SetPixel(wI, f.ColorFromPal(wIndex, false, PALETTE_SOLID_WRAP, 0, wBri));
    }
}

/* Flow Stripe - by ldirko, modified by Andrew Tuline, fixed by DedeHai */
static void FxFlowStripe(Fx& f)
{
    const int      wHl  = std::max(1, f.Len() * 10 / 13);
    const uint8_t  wHue = f.mNow / (f.mSpeed + 1);
    const uint32_t wT   = f.mNow / (f.mIntensity / 8 + 1);

    for (int wI = 0; wI < f.Len(); wI++)
    {
        int wC = (abs(wI - wHl) * 127) / wHl;
        wC = sin8(wC);
        wC = sin8(wC / 2 + wT);
        const uint8_t wB = sin8(wC + wT / 8);
        f.SetPixel(wI, f.ColorFromPal(wB + wHue, false, true, 3));
    }
}

/* Shimmer - by DedeHai */
static void FxShimmer(Fx& f)
{
    const int wLen = f.Len();
    uint32_t* wLastTime = reinterpret_cast<uint32_t*>(f.mData);
    const uint32_t wRadius    = (f.mCustom1 * wLen >> 7) + 1;
    const uint32_t wDistance  = (wLen + 2 * wRadius) << 8;
    const uint32_t wTravTime  = 200 + (255 - f.mSpeed) * 80;
    const uint32_t wSpeed     = (wDistance << 5) / wTravTime;
    int32_t wPosition = static_cast<int32_t>(f.mStep);
    const uint16_t wInput = (uint16_t(f.mIntensity) << 8) | f.mCustom1;

    if ((f.mCall == 0) || (wInput != f.mAux1))
    {
        wPosition = -int32_t(wRadius << 8);
        f.mAux0 = 0;
        *wLastTime = f.mNow;
        f.mAux1 = wInput;
    }

    if (f.mSpeed)
    {
        const uint32_t wDt = (f.mNow - *wLastTime) & 0x7F;
        *wLastTime = f.mNow;
        if (f.mAux0 > 0)
        {
            f.mAux0 = (f.mAux0 > wDt) ? f.mAux0 - wDt : 0;
        }
        else
        {
            wPosition += 1 + ((wSpeed * wDt) >> 5);
            if (wPosition > int32_t((wLen + wRadius) << 8))
            {
                f.mAux0 = f.mIntensity * 236;
                if (f.mCheck3) f.mAux0 = random(f.mAux0 + 1000);
                wPosition = -int32_t(wRadius << 8);
            }
            f.mStep = uint32_t(wPosition);
        }
        if (f.mCheck2) wPosition = (wLen << 8) - wPosition;
    }
    else
    {
        wPosition = wLen << 7;
    }

    for (int wI = 0; wI < wLen; wI++)
    {
        const uint32_t wDist = abs(wPosition - (wI << 8));
        if (wDist < (wRadius << 8))
        {
            CRGB wColor = f.ColorFromPal(wI * 255 / wLen, false, false, 0);
            const uint8_t wBlend = wDist / wRadius;
            if (f.mCustom2)
            {
                uint8_t wMod;
                if (f.mCheck1) wMod = (sin16((wI * f.mCustom2 << 6) + (f.mNow * f.mCustom3 << 5)) >> 8) + 128;
                else           wMod = inoise16((wI * f.mCustom2 << 7), f.mNow * f.mCustom3 << 5) >> 8;
                wColor.nscale8_video(wMod);
            }
            f.SetPixel(wI, blend(wColor, f.mColors[1], wBlend));
        }
        else
        {
            f.SetPixel(wI, f.mColors[1]);
        }
    }
}

/* Color Clouds - ported from https://github.com/JoaDick/EyeCandy */
static void FxColorClouds(Fx& f)
{
    if (f.mCall == 0) { f.mAux0 = random16(); f.mAux1 = random16(); }

    const uint32_t wVolX0 = f.mAux0;
    const uint32_t wHueX0 = f.mAux1;
    const uint8_t  wHueOffset0 = wVolX0 + wHueX0;
    const bool     wCozy = f.mCheck3;
    const uint32_t wVolSpeed   = 1 + f.mSpeed;
    const uint32_t wHueSpeed   = 1 + f.mIntensity;
    const uint32_t wVolSqueeze = 8 + f.mCustom1;
    const uint32_t wHueSqueeze = f.mCustom2;
    const int32_t  wVolCutoff  = 12500 + f.mCustom3 * 900;
    const int32_t  wVolSat     = 52000;
    const uint32_t wVolT = f.mNow * wVolSpeed / 8;
    const uint32_t wHueT = f.mNow * wHueSpeed / 8;
    const uint8_t  wHueOffset = beat88(64) >> 8;

    for (int wI = 0; wI < f.Len(); wI++)
    {
        int32_t wVol = inoise16(wVolX0 + wI * wVolSqueeze * 64, wVolT);
        wVol = map(wVol, wVolCutoff, wVolSat, 0, 255);
        wVol = constrain(wVol, 0, 255);

        uint8_t wHue = inoise16(wHueX0 + wI * wHueSqueeze * 8, wHueT) >> 7;
        wHue += wHueOffset0;
        wHue += wHueOffset;
        if (wCozy) wHue = cos8(128 + wHue / 2);

        CRGB wPixel = f.mPaletteIndex ? f.ColorFromPal(wHue, false, true, 0, wVol) : CRGB(CHSV(wHue, 255, wVol));
        if ((int(wPixel.r) + wPixel.g + wPixel.b) <= 2) wPixel = CRGB::Black;
        f.SetPixel(wI, wPixel);
    }
}


/***************************************************************************************************
  Effect table
 **************************************************************************************************/
/* Palette indices for the WLED effect defaults (see Effects.cpp palette list) */
static constexpr uint8_t PAL_COLORS  = 0;
static constexpr uint8_t PAL_RAINBOW = 1;
static constexpr uint8_t PAL_OCEAN   = 3;
static constexpr uint8_t PAL_FOREST  = 4;
static constexpr uint8_t PAL_HEAT    = 6;
static constexpr uint8_t PAL_STRIPES = 8;
static constexpr uint8_t PAL_C12     = 9;

#define FX1D(name, fn)                   { name, fn, TYPE_1D,      MAP_PIXELS, PAL_COLORS, 128, 128, 16, false, false, false }
#define FX1DM(name, fn, map, pal)        { name, fn, TYPE_1D,      map,        pal,        128, 128, 16, false, false, false }
#define FX2D(name, fn, pal)              { name, fn, TYPE_2D,      MAP_PIXELS, pal,        128, 128, 16, false, false, false }
#define FXCOL(name, fn, pal, c1)         { name, fn, TYPE_1D_COLS, MAP_BAR,    pal,        c1,  128, 16, false, false, false }

/* In WLED-MM effect ID order */
static const tEffectInfo mcEffects1D[] =
{
    FX1D ("Solid",               FxSolid),
    FX1D ("Blink",               FxBlink),
    FX1D ("Breathe",             FxBreathe),
    FX1D ("Wipe",                FxWipe),
    FX1D ("Wipe Random",         FxWipeRandom),
    FX1D ("Random Colors",       FxRandomColors),
    FX1D ("Sweep",               FxSweep),
    FX1D ("Dynamic",             FxDynamic),
    FX1D ("Colorloop",           FxColorloop),
    FX1D ("Rainbow",             FxRainbow),
    FX1D ("Scan",                FxScan),
    FX1D ("Scan Dual",           FxScanDual),
    FX1D ("Fade",                FxFade),
    FX1D ("Theater",             FxTheater),
    FX1D ("Theater Rainbow",     FxTheaterRainbow),
    FX1D ("Running",             FxRunning),
    FX1D ("Saw",                 FxSaw),
    FX1D ("Twinkle",             FxTwinkle),
    FX1D ("Dissolve",            FxDissolve),
    FX1D ("Dissolve Rnd",        FxDissolveRnd),
    FX1D ("Sparkle",             FxSparkle),
    FX1D ("Sparkle Dark",        FxSparkleDark),
    FX1D ("Sparkle+",            FxSparklePlus),
    FX1D ("Strobe",              FxStrobe),
    FX1D ("Strobe Rainbow",      FxStrobeRainbow),
    FX1D ("Strobe Mega",         FxStrobeMega),
    FX1D ("Blink Rainbow",       FxBlinkRainbow),
    FX1DM("Android",             FxAndroid,        MAP_BAR,    PAL_COLORS),
    FX1D ("Chase",               FxChase),
    FX1D ("Chase Random",        FxChaseRandom),
    FX1D ("Chase Rainbow",       FxChaseRainbow),
    FX1D ("Chase Flash",         FxChaseFlash),
    FX1D ("Chase Flash Rnd",     FxChaseFlashRnd),
    FX1D ("Rainbow Runner",      FxRainbowRunner),
    FX1D ("Colorful",            FxColorful),
    FX1D ("Traffic Light",       FxTrafficLight),
    FX1D ("Sweep Random",        FxSweepRandom),
    FX1D ("Chase 2",             FxChase2),
    FX1DM("Aurora",              FxAurora,         MAP_PIXELS, PAL_OCEAN),
    FX1D ("Stream",              FxStream),
    FX1D ("Scanner",             FxScanner),
    FX1D ("Lighthouse",          FxLighthouse),
    FX2D ("Fireworks",           FxFireworks,      PAL_RAINBOW),
    FX2D ("Rain",                FxRain,           PAL_RAINBOW),
    FXCOL("Tetrix",              FxTetrix,         PAL_RAINBOW, 128),
    FX1D ("Fire Flicker",        FxFireFlicker),
    FX1D ("Gradient",            FxGradient),
    FX1D ("Loading",             FxLoading),
    FX1DM("Rolling Balls",       FxRollingBalls,   MAP_BAR,    PAL_COLORS),
    FX1D ("Fairy",               FxFairy),
    FX1D ("Two Dots",            FxTwoDots),
    FX1D ("Fairytwinkle",        FxFairytwinkle),
    FX1D ("Running Dual",        FxRunningDual),
    FX1D ("Chase 3",             FxChase3),
    FX1D ("Tri Wipe",            FxTriWipe),
    FX1D ("Tri Fade",            FxTriFade),
    FX1D ("Lightning",           FxLightning),
    FX1D ("ICU",                 FxIcu),
    FX1D ("Multi Comet",         FxMultiComet),
    FX1D ("Scanner Dual",        FxScannerDual),
    FX1D ("Stream 2",            FxStream2),
    FX1D ("Oscillate",           FxOscillate),
    FX1D ("Pride 2015",          FxPride2015),
    FX1D ("Juggle",              FxJuggle),
    FX1DM("Palette",             FxPalette,        MAP_PIXELS, PAL_COLORS),
    FXCOL("Fire 2012",           FxFire2012,       PAL_HEAT, 128),
    FX1D ("Colorwaves",          FxColorwaves),
    FX1D ("Bpm",                 FxBpm),
    FX1DM("Fill Noise",          FxFillNoise,      MAP_PIXELS, PAL_OCEAN),
    FX1DM("Noise 1",             FxNoise1,         MAP_PIXELS, PAL_RAINBOW),
    FX1DM("Noise 2",             FxNoise2,         MAP_PIXELS, PAL_RAINBOW),
    FX1DM("Noise 3",             FxNoise3,         MAP_PIXELS, PAL_HEAT),
    FX1DM("Noise 4",             FxNoise4,         MAP_PIXELS, PAL_FOREST),
    FX1D ("Colortwinkles",       FxColortwinkles),
    FX1D ("Lake",                FxLake),
    FX1D ("Meteor",              FxMeteor),
    FX1D ("Meteor Smooth",       FxMeteorSmooth),
    FX1DM("Railway",             FxRailway,        MAP_PIXELS, PAL_C12),
    FX2D ("Ripple",              FxRipple,         PAL_COLORS),
    FX1D ("Twinklefox",          FxTwinklefox),
    FX1D ("Twinklecat",          FxTwinklecat),
    FX2D ("Halloween Eyes",      FxHalloweenEyes,  PAL_COLORS),
    FX1D ("Solid Pattern",       FxSolidPattern),
    FX1D ("Solid Pattern Tri",   FxSolidPatternTri),
    FX1D ("Spots",               FxSpots),
    FX1D ("Spots Fade",          FxSpotsFade),
    FX1DM("Glitter",             FxGlitter,        MAP_PIXELS, PAL_RAINBOW),
    FX1D ("Candle",              FxCandle),
    FX1DM("Fireworks Starburst", FxStarburst,      MAP_PIXELS, PAL_RAINBOW),
    FX2D ("Fireworks 1D",        FxFireworks1D,    PAL_RAINBOW),
    FXCOL("Bouncing Balls",      FxBouncingBalls,  PAL_COLORS, 128),
    FX1D ("Sinelon",             FxSinelon),
    FX1D ("Sinelon Dual",        FxSinelonDual),
    FX1D ("Sinelon Rainbow",     FxSinelonRainbow),
    FXCOL("Popcorn",             FxPopcorn,        PAL_COLORS, 128),
    FXCOL("Drip",                FxDrip,           PAL_COLORS, 127),
    FX1D ("Plasma",              FxPlasma),
    FX1D ("Percent",             FxPercent),
    FX2D ("Ripple Rainbow",      FxRippleRainbow,  PAL_COLORS),
    FX1D ("Heartbeat",           FxHeartbeat),
    FX1D ("Pacifica",            FxPacifica),
    FX1D ("Candle Multi",        FxCandleMulti),
    FX1D ("Solid Glitter",       FxSolidGlitter),
    FX1DM("Sunrise",             FxSunrise,        MAP_PIXELS, PAL_HEAT),
    FX1D ("Phased",              FxPhased),
    FX1D ("Twinkleup",           FxTwinkleup),
    FX1D ("Noise Pal",           FxNoisePal),
    FX1D ("Sine",                FxSine),
    FX1D ("Phased Noise",        FxPhasedNoise),
    FX1DM("Flow",                FxFlow,           MAP_BAR,    PAL_COLORS),
    FX1D ("Chunchun",            FxChunchun),
    FX1D ("Dancing Shadows",     FxDancingShadows),
    FX1D ("Washing Machine",     FxWashingMachine),
    FX1D ("Blends",              FxBlends),
    FX1D ("TV Simulator",        FxTvSimulator),
    FX1D ("Dynamic Smooth",      FxDynamicSmooth),
    FX1D ("Perlin Move",         FxPerlinMove),
    { "Shimmer",     FxShimmer,     TYPE_1D, MAP_PIXELS, PAL_STRIPES, 128,  0,  0, false, false, false },
    FX1DM("Flow Stripe",         FxFlowStripe,     MAP_PIXELS, PAL_RAINBOW),
    FX1D ("Wavesins",            FxWavesins),
    { "Color Clouds", FxColorClouds, TYPE_1D, MAP_PIXELS, PAL_COLORS,   48, 64, 12, false, false, false },
};

#undef FX1D
#undef FX1DM
#undef FX2D
#undef FXCOL

/* Per-effect state must fit into the data buffer */
static_assert(sizeof(tTetris) * Fx::mCols <= mcDataSize,             "Effects: data buffer too small (Tetrix)");
static_assert(Fx::mCols * Fx::mRows <= mcDataSize,                   "Effects: data buffer too small (Fire 2012)");
static_assert(sizeof(tBall) * 16 * Fx::mCols <= mcDataSize,          "Effects: data buffer too small (Bouncing Balls)");
static_assert(sizeof(tSparkDrop) * 4 * Fx::mCols <= mcDataSize,      "Effects: data buffer too small (Drip)");
static_assert(sizeof(tFlasher) * Fx::mCols * Fx::mRows <= mcDataSize, "Effects: data buffer too small (Fairytwinkle)");
static_assert(3 * Fx::mCols * Fx::mRows <= mcDataSize,               "Effects: data buffer too small (Candle Multi)");
static_assert(sizeof(CRGB) * 256 <= mcDataSize,                      "Effects: data buffer too small (Blends)");
static_assert(sizeof(tAuroraWave) * mcAuroraMaxWaves <= mcDataSize,  "Effects: data buffer too small (Aurora)");
static_assert(sizeof(tRipple) * 100 <= mcDataSize,                   "Effects: data buffer too small (Ripple)");
static_assert(sizeof(tTvSim) <= mcDataSize,                          "Effects: data buffer too small (TV Simulator)");
static_assert(2 * sizeof(CRGBPalette16) <= mcDataSize,               "Effects: data buffer too small (Noise Pal)");

const tEffectInfo* GetEffects1D(uint8_t& arCount)
{
    arCount = sizeof(mcEffects1D) / sizeof(mcEffects1D[0]);
    return mcEffects1D;
}

}; /* end of namespace EffectsNS */
