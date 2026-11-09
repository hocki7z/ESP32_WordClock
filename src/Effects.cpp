/*
 * Effects.cpp
 *
 *  Created on: 09.11.2026
 *      Author: hocki
 *
 *  2D matrix effects ported from WLED-MM (https://github.com/MoonModules/WLED-MM, wled00/FX.cpp, EUPL-1.2).
 *  Original authors are named at each effect. Sound-reactive and particle-system effects are not included.
 *  1D effects are in Effects1D.cpp.
 */

#include <Arduino.h>
#include <algorithm>

#include "Effects.h"
#include "Font5x7.h"


namespace EffectsNS
{

/***************************************************************************************************
  Canvas helpers
 **************************************************************************************************/
void Fx::SetXY(int aX, int aY, const CRGB& arColor)
{
    if (Inside(aX, aY)) mLeds[XY(aX, aY)] = arColor;
}

CRGB Fx::GetXY(int aX, int aY) const
{
    return Inside(aX, aY) ? mLeds[XY(aX, aY)] : CRGB(CRGB::Black);
}

void Fx::AddXY(int aX, int aY, const CRGB& arColor)
{
    if (Inside(aX, aY)) mLeds[XY(aX, aY)] += arColor;
}

void Fx::BlendXY(int aX, int aY, const CRGB& arColor, uint8_t aAmount)
{
    if (Inside(aX, aY)) nblend(mLeds[XY(aX, aY)], arColor, aAmount);
}

void Fx::FadeXY(int aX, int aY, uint8_t aScale)
{
    if (Inside(aX, aY)) mLeds[XY(aX, aY)].nscale8_video(aScale);
}

void Fx::Fill(const CRGB& arColor)
{
    fill_solid(Buf(), BufLen(), arColor);
}

void Fx::FadeAll(uint8_t aAmount)
{
    fadeToBlackBy(Buf(), BufLen(), aAmount);
}

void Fx::FadeOut(uint8_t aRate)
{
    const float wRate = float((255 - aRate) >> 1) + 1.1f;
    const CRGB  wT    = mColors[1];
    CRGB*       wBuf  = Buf();

    auto wStep = [wRate](uint8_t aFrom, uint8_t aTo) -> uint8_t
    {
        int wDelta = (int(aTo) - int(aFrom)) / wRate;
        wDelta += (aTo == aFrom) ? 0 : ((aTo > aFrom) ? 1 : -1);
        return static_cast<uint8_t>(aFrom + wDelta);
    };

    for (int wI = 0; wI < BufLen(); wI++)
    {
        CRGB& wC = wBuf[wI];
        if (wC == wT) continue;
        wC = CRGB(wStep(wC.r, wT.r), wStep(wC.g, wT.g), wStep(wC.b, wT.b));
    }
}

void Fx::SetPixel(int aIndex, const CRGB& arColor)
{
    if (aIndex < 0) return;
    const int wIdx   = aIndex & 0xFFFF;
    const int wStrip = aIndex >> 16;

    if (mType == TYPE_1D)
    {
        if (wIdx < mLen) mStrip[wIdx] = arColor;
    }
    else if (mType == TYPE_1D_COLS)
    {
        if (wIdx >= mRows) return;
        if (wStrip == 0) for (int wX = 0; wX < mCols; wX++) mLeds[XY(wX, mRows - 1 - wIdx)] = arColor;
        else             SetXY(wStrip - 1, mRows - 1 - wIdx, arColor);
    }
    else
    {
        if (wIdx < mCols * mRows) mLeds[wIdx] = arColor;
    }
}

CRGB Fx::GetPixel(int aIndex) const
{
    if (aIndex < 0) return CRGB::Black;
    const int wIdx   = aIndex & 0xFFFF;
    const int wStrip = aIndex >> 16;

    if (mType == TYPE_1D)            return (wIdx < mLen) ? mStrip[wIdx] : CRGB(CRGB::Black);
    else if (mType == TYPE_1D_COLS)  return GetXY((wStrip > 0) ? (wStrip - 1) : 0, mRows - 1 - wIdx);
    else                             return (wIdx < mCols * mRows) ? mLeds[wIdx] : CRGB(CRGB::Black);
}

void Fx::AddPixel(int aIndex, const CRGB& arColor)
{
    SetPixel(aIndex, GetPixel(aIndex) + arColor);
}

void Fx::BlendPixel(int aIndex, const CRGB& arColor, uint8_t aAmount)
{
    SetPixel(aIndex, blend(GetPixel(aIndex), arColor, aAmount));
}

CRGB Fx::ColorFromPal(uint32_t aIndex, bool aMapping, bool aWrap, uint8_t aColorSlot, uint8_t aBrightness) const
{
    if ((mPaletteIndex == 0) && (aColorSlot < 3))
    {
        CRGB wColor = mColors[aColorSlot];
        if (aBrightness != 255) wColor.nscale8_video(aBrightness);
        return wColor;
    }

    uint8_t wIndex = aIndex;
    if (aMapping && (mLen > 1)) wIndex = (aIndex * 255) / (mLen - 1);
    if (!aWrap) wIndex = scale8(wIndex, 240);
    return ColorFromPalette(mPalette, wIndex, aBrightness, LINEARBLEND);
}

CRGB Fx::Wheel(uint8_t aPos) const
{
    if (mPaletteIndex != 0) return ColorFromPal(aPos, false, true, 0);

    aPos = 255 - aPos;
    if (aPos < 85)  return CRGB(255 - aPos * 3, 0, aPos * 3);
    if (aPos < 170) { aPos -= 85; return CRGB(0, aPos * 3, 255 - aPos * 3); }
    aPos -= 170;
    return CRGB(aPos * 3, 255 - aPos * 3, 0);
}

uint8_t Fx::RandomWheelIndex(uint8_t aPos) const
{
    uint8_t wR = 0, wD = 0;
    while (wD < 42)
    {
        wR = random8();
        const uint8_t wX = abs(int(aPos) - int(wR));
        wD = std::min<uint8_t>(wX, 255 - wX);
    }
    return wR;
}

void Fx::Project(void)
{
    if (mType != TYPE_1D) return;

    const float wCx = (mCols - 1) / 2.0f;
    const float wCy = (mRows - 1) / 2.0f;

    for (int wY = 0; wY < mRows; wY++)
    {
        for (int wX = 0; wX < mCols; wX++)
        {
            int wIdx;
            switch (mMapping)
            {
                case MAP_BAR:    wIdx = mRows - 1 - wY; break;
                case MAP_CIRCLE: wIdx = int(sqrtf((wX - wCx) * (wX - wCx) + (wY - wCy) * (wY - wCy))); break;
                case MAP_CORNER: wIdx = std::max(wX, wY); break;
                default:         wIdx = XY(wX, wY); break;
            }
            mLeds[XY(wX, wY)] = (wIdx < mLen) ? mStrip[wIdx] : CRGB(CRGB::Black);
        }
    }
}

CRGB Fx::Pal(uint8_t aIndex, uint8_t aBrightness) const
{
    return ColorFromPalette(mPalette, aIndex, aBrightness, LINEARBLEND);
}

void Fx::BlurLine(CRGB* apBuf, int aStart, int aStride, int aCount, uint8_t aAmount)
{
    if (aAmount == 0) return;

    const uint8_t wKeep = 255 - aAmount;
    const uint8_t wSeep = aAmount >> 1;
    CRGB wCarry = CRGB::Black;

    for (int wI = 0; wI < aCount; wI++)
    {
        CRGB& wCur  = apBuf[aStart + wI * aStride];
        CRGB  wPart = wCur;
        wPart.nscale8(wSeep);
        wCur.nscale8(wKeep);
        wCur += wCarry;
        if (wI > 0) apBuf[aStart + (wI - 1) * aStride] += wPart;
        wCarry = wPart;
    }
}

void Fx::BlurRows(uint8_t aAmount)
{
    if (mType == TYPE_1D) { BlurLine(mStrip, 0, 1, mLen, aAmount); return; }
    for (int wY = 0; wY < mRows; wY++) BlurLine(mLeds, wY * mCols, 1, mCols, aAmount);
}

void Fx::BlurCols(uint8_t aAmount)
{
    if (mType == TYPE_1D) return;
    for (int wX = 0; wX < mCols; wX++) BlurLine(mLeds, wX, mCols, mRows, aAmount);
}

void Fx::Blur(uint8_t aAmount)
{
    BlurRows(aAmount);
    BlurCols(aAmount);
}

void Fx::DrawLine(int aX0, int aY0, int aX1, int aY1, const CRGB& arColor)
{
    const int wDx = abs(aX1 - aX0), wSx = (aX0 < aX1) ? 1 : -1;
    const int wDy = -abs(aY1 - aY0), wSy = (aY0 < aY1) ? 1 : -1;
    int wErr = wDx + wDy;

    for (;;)
    {
        SetXY(aX0, aY0, arColor);
        if ((aX0 == aX1) && (aY0 == aY1)) break;
        const int wE2 = 2 * wErr;
        if (wE2 >= wDy) { wErr += wDy; aX0 += wSx; }
        if (wE2 <= wDx) { wErr += wDx; aY0 += wSy; }
    }
}

void Fx::DrawCircle(int aCx, int aCy, int aRadius, const CRGB& arColor)
{
    int wX = aRadius, wY = 0, wErr = 1 - aRadius;
    while (wX >= wY)
    {
        SetXY(aCx + wX, aCy + wY, arColor); SetXY(aCx - wX, aCy + wY, arColor);
        SetXY(aCx + wX, aCy - wY, arColor); SetXY(aCx - wX, aCy - wY, arColor);
        SetXY(aCx + wY, aCy + wX, arColor); SetXY(aCx - wY, aCy + wX, arColor);
        SetXY(aCx + wY, aCy - wX, arColor); SetXY(aCx - wY, aCy - wX, arColor);
        wY++;
        if (wErr < 0) wErr += 2 * wY + 1;
        else          { wX--; wErr += 2 * (wY - wX) + 1; }
    }
}

void Fx::FillCircle(int aCx, int aCy, int aRadius, const CRGB& arColor)
{
    for (int wY = -aRadius; wY <= aRadius; wY++)
    {
        for (int wX = -aRadius; wX <= aRadius; wX++)
        {
            if ((wX * wX + wY * wY) <= (aRadius * aRadius)) SetXY(aCx + wX, aCy + wY, arColor);
        }
    }
}

void Fx::WuPixel(uint32_t aX, uint32_t aY, const CRGB& arColor)
{
    const uint8_t wXx = aX & 0xFF, wYy = aY & 0xFF;
    const uint8_t wIx = 255 - wXx,  wIy = 255 - wYy;
    auto wWeight = [](uint8_t aA, uint8_t aB) { return static_cast<uint8_t>((aA * aB + aA + aB) >> 8); };
    const uint8_t wWu[4] = { wWeight(wIx, wIy), wWeight(wXx, wIy), wWeight(wIx, wYy), wWeight(wXx, wYy) };

    for (int wI = 0; wI < 4; wI++)
    {
        const int wPx = static_cast<int>(aX >> 8) + (wI & 1);
        const int wPy = static_cast<int>(aY >> 8) + ((wI >> 1) & 1);
        if (!Inside(wPx, wPy)) continue;

        CRGB& wLed = mLeds[XY(wPx, wPy)];
        wLed.r = qadd8(wLed.r, (arColor.r * wWu[wI]) >> 8);
        wLed.g = qadd8(wLed.g, (arColor.g * wWu[wI]) >> 8);
        wLed.b = qadd8(wLed.b, (arColor.b * wWu[wI]) >> 8);
    }
}

void Fx::MoveX(int aDelta, bool aWrap)
{
    if (aDelta == 0) return;
    for (int wY = 0; wY < mRows; wY++)
    {
        CRGB wRow[mCols];
        for (int wX = 0; wX < mCols; wX++)
        {
            const int wSx = wX + aDelta;
            wRow[wX] = aWrap ? mLeds[XY(((wSx % mCols) + mCols) % mCols, wY)] : GetXY(wSx, wY);
        }
        for (int wX = 0; wX < mCols; wX++) mLeds[XY(wX, wY)] = wRow[wX];
    }
}

void Fx::MoveY(int aDelta, bool aWrap)
{
    if (aDelta == 0) return;
    for (int wX = 0; wX < mCols; wX++)
    {
        CRGB wCol[mRows];
        for (int wY = 0; wY < mRows; wY++)
        {
            const int wSy = wY + aDelta;
            wCol[wY] = aWrap ? mLeds[XY(wX, ((wSy % mRows) + mRows) % mRows)] : GetXY(wX, wSy);
        }
        for (int wY = 0; wY < mRows; wY++) mLeds[XY(wX, wY)] = wCol[wY];
    }
}

void Fx::Move(uint8_t aDirection, uint8_t aDelta, bool aWrap)
{
    static const int8_t wcDx[8] = { 1, 1, 0, -1, -1, -1,  0,  1 };
    static const int8_t wcDy[8] = { 0, 1, 1,  1,  0, -1, -1, -1 };

    MoveX(wcDx[aDirection & 7] * aDelta, aWrap);
    MoveY(wcDy[aDirection & 7] * aDelta, aWrap);
}


/***************************************************************************************************
  Effects
 **************************************************************************************************/
static constexpr int mcCols = Fx::mCols;
static constexpr int mcRows = Fx::mRows;

/* WLED frame time in ms */
static constexpr uint32_t mcFrameTime = 24;

static const CRGBPalette16 mcFirePalette(
    CRGB::Black,      CRGB::Black,      CRGB::Black,  CRGB::Black,
    CRGB::Red,        CRGB::Red,        CRGB::Red,    CRGB::DarkOrange,
    CRGB::DarkOrange, CRGB::DarkOrange, CRGB::Orange, CRGB::Orange,
    CRGB::Yellow,     CRGB::Orange,     CRGB::Yellow, CRGB::Yellow);

static const CRGBPalette16 mcAuroraPalette(
    CRGB(0x000000), CRGB(0x003300), CRGB(0x006600), CRGB(0x009900),
    CRGB(0x00cc00), CRGB(0x00ff00), CRGB(0x33ff00), CRGB(0x66ff00),
    CRGB(0x99ff00), CRGB(0xccff00), CRGB(0xffff00), CRGB(0xffcc00),
    CRGB(0xff9900), CRGB(0xff6600), CRGB(0xff3300), CRGB(0xff0000));


/* Black Hole - by Stepko, modified by Andrew Tuline */
static void FxBlackHole(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);

    f.FadeAll(16 + (f.mSpeed >> 3));
    const uint32_t wT = f.mNow;
    constexpr uint32_t wRatio = 128;

    for (uint32_t wI = 0; wI < 8; wI++)
    {
        const int wX = beatsin8(f.mCustom1 >> 3,   0, mcCols - 1, 0, ((wI % 2) ? 128 : 0) + (wT * wI) / wRatio);
        const int wY = beatsin8(f.mIntensity >> 3, 0, mcRows - 1, 0, ((wI % 2) ? 192 : 64) + (wT * wI) / wRatio);
        f.AddXY(wX, wY, CHSV(wI * 32, 255, 255));
    }
    for (uint32_t wI = 0; wI < 4; wI++)
    {
        const int wX = beatsin8(f.mCustom2 >> 3, mcCols / 4, mcCols - 1 - mcCols / 4, 0, ((wI % 2) ? 128 : 0) + (wT * wI) / wRatio);
        const int wY = beatsin8(f.mCustom3,      mcRows / 4, mcRows - 1 - mcRows / 4, 0, ((wI % 2) ? 192 : 64) + (wT * wI) / wRatio);
        f.AddXY(wX, wY, CHSV(wI * 32, 255, 255));
    }
    f.SetXY(mcCols / 2, mcRows / 2, CRGB::White);
    f.Blur(16);
}

/* Colored Bursts - by ldirko, modified by Andrew Tuline */
static void FxColoredBursts(Fx& f)
{
    if (f.mCall == 0) { f.Fill(CRGB::Black); f.mAux0 = 0; }

    const uint8_t wNumLines = f.mIntensity / 16 + 1;
    f.mAux0++;
    f.FadeAll(40);

    for (uint8_t wI = 0; wI < wNumLines; wI++)
    {
        const uint8_t wX1 = beatsin8(2 + f.mSpeed / 16, 0, mcCols - 1);
        const uint8_t wX2 = beatsin8(1 + f.mSpeed / 16, 0, mcRows - 1);
        const uint8_t wY1 = beatsin8(5 + f.mSpeed / 16, 0, mcCols - 1, 0, wI * 24);
        const uint8_t wY2 = beatsin8(3 + f.mSpeed / 16, 0, mcRows - 1, 0, wI * 48 + 64);
        const CRGB wColor = f.Pal(wI * 255 / wNumLines + (f.mAux0 & 0xFF));

        const uint8_t wXSteps = abs(wX1 - wY1) + 1;
        const uint8_t wYSteps = abs(wX2 - wY2) + 1;
        const uint8_t wSteps  = max(wXSteps, wYSteps);

        for (uint8_t wJ = 1; wJ <= wSteps; wJ++)
        {
            const uint8_t wRate = wJ * 255 / wSteps;
            const uint8_t wDx = lerp8by8(wX1, wY1, wRate);
            const uint8_t wDy = lerp8by8(wX2, wY2, wRate);
            f.AddXY(wDx, wDy, wColor);
            if (f.mCheck1) f.FadeXY(wDx, wDy, dim8_raw(wRate));
        }
        if (f.mCheck3)
        {
            f.SetXY(wX1, wX2, CRGB::White);
            f.SetXY(wY1, wY2, CRGB::DarkSlateGray);
        }
    }
    if (f.mCustom3) f.Blur(f.mCustom3 / 2);
}

/* DNA - by ldirko, updated by Preyy, WLED conversion by Andrew Tuline */
static void FxDna(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);
    f.FadeAll(64);

    for (int wI = 0; wI < mcCols; wI++)
    {
        const unsigned wPhase = wI * 4 * f.mCustom1 / 128;
        const int wPosY1 = beatsin8(f.mSpeed / 8, 0, mcRows - 1, 0, wPhase);
        const int wPosY2 = beatsin8(f.mSpeed / 8, 0, mcRows - 1, 0, wPhase + 128);
        f.SetXY(wI, wPosY1, f.Pal(wI * 5 + f.mNow / 17,       beatsin8(5, 55, 255, 0, wI * 10)));
        f.SetXY(wI, wPosY2, f.Pal(wI * 5 + 128 + f.mNow / 17, beatsin8(5, 55, 255, 0, wI * 10 + 128)));
    }
    f.Blur(f.mIntensity >> 3);
}

/* DNA Spiral - by ldirko, modified by Andrew Tuline */
static void FxDnaSpiral(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);

    const uint8_t  wSpeeds = f.mSpeed / 2 + 7;
    const uint8_t  wFreq   = f.mIntensity / 8;
    const uint32_t wMs     = f.mNow / 20;
    f.FadeAll(135);

    for (int wI = 0; wI < mcRows; wI++)
    {
        int wX  = beatsin8(wSpeeds, 0, mcCols - 1, 0, wI * wFreq)       + beatsin8(wSpeeds - 7, 0, mcCols - 1, 0, wI * wFreq + 128);
        int wX1 = beatsin8(wSpeeds, 0, mcCols - 1, 0, 128 + wI * wFreq) + beatsin8(wSpeeds - 7, 0, mcCols - 1, 0, 128 + 64 + wI * wFreq);
        const uint8_t wHue = (wI * 128 / mcRows) + wMs;

        if ((wI + wMs / 8) & 3)
        {
            wX /= 2; wX1 /= 2;
            const int  wSteps    = abs(wX - wX1) + 1;
            const bool wPositive = (wX1 >= wX);
            for (int wK = 1; wK <= wSteps; wK++)
            {
                const uint8_t wRate = wK * 255 / wSteps;
                const int     wDx   = wPositive ? (wX + wK - 1) : (wX - wK + 1);
                f.AddXY(wDx, wI, f.Pal(wHue));
                f.FadeXY(wDx, wI, wRate);
            }
            f.SetXY(wX,  wI, CRGB::DarkSlateGray);
            f.SetXY(wX1, wI, CRGB::White);
        }
    }
}

/* Drift - by Stepko, modified by Andrew Tuline */
static void FxDrift(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);

    if (f.mIntensity > 1) f.FadeAll(128);
    else                  f.Fill(CRGB::Black);

    const float wMaxDim = max(mcCols, mcRows) / 2.0f;
    const float wT      = float(f.mNow & 0x003FFFFF) / float(32U - (f.mSpeed >> 3));
    const int   wColsC  = (mcCols >> 1) + (mcCols % 2);
    const int   wRowsC  = (mcRows >> 1) + (mcRows % 2);
    const unsigned wT20 = wT / 20.0f;

    for (float wI = 1.0f; wI <= wMaxDim; wI += 0.25f)
    {
        float wAngle = float(DEG_TO_RAD) * wT * (wMaxDim - wI);
        wAngle = fmodf(wAngle, float(TWO_PI));
        const int wSin = sinf(wAngle) * wI;
        const int wCos = cosf(wAngle) * wI;
        const CRGB wColor = f.Pal(static_cast<uint8_t>(unsigned(wI * 20.0f) + wT20));
        f.SetXY(wColsC + wSin, wRowsC + wCos, wColor);
        if (f.mCheck1) f.SetXY(wColsC + wCos, wRowsC + wSin, wColor);
    }
    f.Blur(f.mIntensity >> (f.mCheck2 ? 0 : 3));
}

/* Firenoise - by Andrew Tuline */
static void FxFirenoise(Fx& f)
{
    const unsigned wXScale = f.mIntensity * 4;
    const unsigned wYScale = f.mSpeed * 8;
    const CRGBPalette16& wPal = ((f.mPaletteIndex != 0) || f.mCheck1) ? f.mPalette : mcFirePalette;

    for (int wJ = 0; wJ < mcCols; wJ++)
    {
        for (int wI = 0; wI < mcRows; wI++)
        {
            const unsigned wIndex = inoise8(wJ * wYScale * mcRows / 255, wI * wXScale + f.mNow / 4);
            f.SetXY(wJ, wI, ColorFromPalette(wPal, min(wI * wIndex >> 4, 255U), wI * 255 / mcCols, LINEARBLEND));
        }
    }
}

/* Frizzles - by Stepko, modified by Andrew Tuline */
static void FxFrizzles(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);

    f.FadeAll(16);
    for (int wI = 8; wI > 0; wI--)
    {
        f.AddXY(beatsin8(f.mSpeed / 8 + wI, 0, mcCols - 1),
                beatsin8(static_cast<uint16_t>(f.mIntensity / 8 - wI), 0, mcRows - 1),
                f.Pal(beatsin8(12, 0, 255)));
    }
    f.Blur(f.mCustom1 >> 3);
}

/* Game Of Life - simplified port of the WLED-MM effect by Ewoud Wijma and Brandon Butler */
static void FxGameOfLife(Fx& f)
{
    constexpr int N = mcCols * mcRows;
    uint8_t*  wCells = f.mData;
    uint8_t*  wNext  = f.mData + N;
    uint32_t* wHist  = reinterpret_cast<uint32_t*>(f.mData + 2 * N);
    uint16_t& wGeneration = f.mAux0;

    if (f.mCall == 0) wGeneration = 0;
    if (static_cast<int32_t>(f.mNow - f.mStep) < 0) return;

    /* New game */
    if (wGeneration == 0)
    {
        f.Fill(CRGB::Black);
        for (int wI = 0; wI < N; wI++)
        {
            wCells[wI] = (random8() < 82) ? 1 : 0;   // ~32% alive
            if (wCells[wI]) f.mLeds[wI] = f.Pal(random8());
        }
        wHist[0] = wHist[1] = 0;
        wGeneration = 1;
        f.mStep = f.mNow + 1250;
        return;
    }

    const bool wWrap = f.mCheck3;
    uint32_t wHash  = 2166136261u;
    uint16_t wAlive = 0;

    /* Apply rules */
    for (int wY = 0; wY < mcRows; wY++)
    {
        for (int wX = 0; wX < mcCols; wX++)
        {
            uint8_t wN = 0;
            for (int wDy = -1; wDy <= 1; wDy++)
            {
                for (int wDx = -1; wDx <= 1; wDx++)
                {
                    if ((wDx == 0) && (wDy == 0)) continue;
                    int wNx = wX + wDx, wNy = wY + wDy;
                    if (wWrap) { wNx = (wNx + mcCols) % mcCols; wNy = (wNy + mcRows) % mcRows; }
                    else if (!Fx::Inside(wNx, wNy)) continue;
                    wN += wCells[Fx::XY(wNx, wNy)];
                }
            }
            const int  wIdx   = Fx::XY(wX, wY);
            const bool wLives = wCells[wIdx] ? ((wN == 2) || (wN == 3)) : (wN == 3);
            wNext[wIdx] = wLives ? 1 : 0;
            if (wLives) { wAlive++; wHash = (wHash ^ (wIdx + 1)) * 16777619u; }
        }
    }

    /* Births: inherit a neighbor color, or mutate */
    for (int wY = 0; wY < mcRows; wY++)
    {
        for (int wX = 0; wX < mcCols; wX++)
        {
            const int wIdx = Fx::XY(wX, wY);
            if (!wNext[wIdx] || wCells[wIdx]) continue;

            CRGB wColors[8];
            uint8_t wCount = 0;
            for (int wDy = -1; wDy <= 1; wDy++)
            {
                for (int wDx = -1; wDx <= 1; wDx++)
                {
                    int wNx = (wX + wDx + mcCols) % mcCols, wNy = (wY + wDy + mcRows) % mcRows;
                    if (((wDx != 0) || (wDy != 0)) && wCells[Fx::XY(wNx, wNy)]) wColors[wCount++] = f.mLeds[Fx::XY(wNx, wNy)];
                }
            }
            f.mLeds[wIdx] = ((wCount == 0) || (random8() < (f.mIntensity >> 3))) ? f.Pal(random8()) : wColors[random8(wCount)];
        }
    }

    /* Dead cells fade out */
    for (int wI = 0; wI < N; wI++)
    {
        if (!wNext[wI]) f.mLeds[wI].nscale8(f.mCustom1);
        wCells[wI] = wNext[wI];
    }

    /* Restart on extinction, still life, period-2 oscillation or after a long run */
    if ((wAlive == 0) || (wHash == wHist[0]) || (wHash == wHist[1]) || (wGeneration > 2000))
    {
        wGeneration = 0;
        f.mStep = f.mNow + 1000;
    }
    else
    {
        wHist[1] = wHist[0];
        wHist[0] = wHash;
        wGeneration++;
        f.mStep = f.mNow + 1000 / map(f.mSpeed, 0, 255, 1, 60);
    }
}

/* Hiphotic - by ldirko, modified by Andrew Tuline */
static void FxHiphotic(Fx& f)
{
    const uint32_t wA = f.mNow / ((f.mCustom3 >> 1) + 1);

    for (int wX = 0; wX < mcCols; wX++)
    {
        for (int wY = 0; wY < mcRows; wY++)
        {
            f.SetXY(wX, wY, f.Pal(sin8(cos8(wX * f.mSpeed / 16 + wA / 3) + sin8(wY * f.mIntensity / 16 + wA / 4) + wA)));
        }
    }
}

/* Julia - animated Julia set by Andrew Tuline */
struct tJulia { float mXCen; float mYCen; float mXYMag; };

static void FxJulia(Fx& f)
{
    tJulia* wJ = reinterpret_cast<tJulia*>(f.mData);

    if (f.mCall == 0)
    {
        f.Fill(CRGB::Black);
        wJ->mXCen = 0.0f; wJ->mYCen = 0.0f; wJ->mXYMag = 1.0f;
    }

    constexpr float wMaxCenter = 2.5f;
    if (fabsf(wJ->mXCen) < wMaxCenter) wJ->mXCen += float(f.mCustom1 - 128) / 100000.f;
    if (fabsf(wJ->mYCen) < wMaxCenter) wJ->mYCen += float(f.mCustom2 - 128) / 100000.f;
    wJ->mXYMag += float((f.mCustom3 - 16) * 8) / 100000.f;
    wJ->mXYMag = constrain(wJ->mXYMag, 0.01f, 1.0f);

    const float wXMin = constrain(wJ->mXCen - wJ->mXYMag, -1.2f, 1.2f);
    const float wXMax = constrain(wJ->mXCen + wJ->mXYMag, -1.2f, 1.2f);
    const float wYMin = constrain(wJ->mYCen - wJ->mXYMag, -0.8f, 1.0f);
    const float wYMax = constrain(wJ->mYCen + wJ->mXYMag, -0.8f, 1.0f);

    const int   wMaxIter = max(1, f.mIntensity / 2);
    const float wMaxCalc = 16.0f;
    const float wReAl = -0.94299f + float(sin16(f.mNow * 34)) / 655340.f;
    const float wImAg =  0.3162f  + float(sin16(f.mNow * 26)) / 655340.f;
    const float wDx = (wXMax - wXMin) / mcCols;
    const float wDy = (wYMax - wYMin) / mcRows;

    float wY = wYMin;
    for (int wRow = 0; wRow < mcRows; wRow++)
    {
        float wX = wXMin;
        for (int wCol = 0; wCol < mcCols; wCol++)
        {
            float wA = wX, wB = wY;
            int wIter = 0;
            while (wIter < wMaxIter)
            {
                const float wAa = wA * wA, wBb = wB * wB;
                if ((wAa + wBb) > wMaxCalc) break;
                wB = 2 * wA * wB + wImAg;
                wA = wAa - wBb + wReAl;
                wIter++;
            }
            f.SetXY(wCol, wRow, (wIter == wMaxIter) ? CRGB(CRGB::Black) : f.Pal(wIter * 255 / wMaxIter));
            wX += wDx;
        }
        wY += wDy;
    }

    if (f.mCheck1) f.BlurRows(48);
    if (f.mCheck2) f.Blur(64);
}

/* Lissajous - by Andrew Tuline */
static void FxLissajous(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);
    f.FadeAll(f.mIntensity);

    const uint32_t wPhase = (f.mNow * (1 + f.mCustom3)) / 32;
    for (int wI = 0; wI < 256; wI++)
    {
        uint8_t wXl = sin8(wPhase / 2 + (wI * f.mSpeed) / 64);
        uint8_t wYl = cos8(wPhase / 2 + wI * 2);
        wXl = (map(2 * wXl, 0, 511, 0, 2 * (mcCols - 1)) + 1) / 2;
        wYl = (map(2 * wYl, 0, 511, 0, 2 * (mcRows - 1)) + 1) / 2;
        f.SetXY(wXl, wYl, f.Pal(f.mNow / 100 + wI));
    }
}

/* Matrix - by Jeremy Williams, adapted by Andrew Tuline, improved by merkisoft, ewowi and softhack007 */
static void FxMatrix(Fx& f)
{
    if (f.mCall == 0) { f.Fill(CRGB::Black); f.mStep = 0; }

    const uint8_t wFade  = map(f.mCustom1, 0, 255, 50, 250);
    const uint32_t wDelay = 256 - f.mSpeed;
    const CRGB wSpawnColor(175, 255, 175);
    const CRGB wTrailColor(27, 130, 39);

    if ((f.mNow - f.mStep) < wDelay) return;
    f.mStep = f.mNow;

    bool wEmpty = true;
    for (int wRow = mcRows - 1; wRow >= 0; wRow--)
    {
        for (int wCol = 0; wCol < mcCols; wCol++)
        {
            CRGB wPix = f.GetXY(wCol, wRow);
            if (wPix == wSpawnColor)
            {
                f.SetXY(wCol, wRow, wTrailColor);
                if (wRow < mcRows - 1) { f.SetXY(wCol, wRow + 1, wSpawnColor); wEmpty = false; }
            }
            else if (wPix != CRGB(CRGB::Black))
            {
                f.SetXY(wCol, wRow, wPix.nscale8(wFade));
            }
        }
    }

    if ((random8() <= f.mIntensity) || wEmpty)
    {
        f.SetXY(random16(mcCols), 0, wSpawnColor);
    }
}

/* Metaballs - by Stefan Petrick, adapted by Andrew Tuline */
static void FxMetaballs(Fx& f)
{
    const float wSpeed = 0.25f * (1 + (f.mSpeed >> 6));
    const uint16_t wT = static_cast<uint16_t>(f.mNow * wSpeed);

    const uint8_t wX2 = map(inoise8(wT, 25355,   685), 0, 255, 0, mcCols - 1);
    const uint8_t wY2 = map(inoise8(wT,   355, 11685), 0, 255, 0, mcRows - 1);
    const uint8_t wX3 = map(inoise8(wT, 55355,  6685), 0, 255, 0, mcCols - 1);
    const uint8_t wY3 = map(inoise8(wT, 25355, 22685), 0, 255, 0, mcRows - 1);
    const uint8_t wX1 = beatsin8(23 * wSpeed, 0, mcCols - 1);
    const uint8_t wY1 = beatsin8(28 * wSpeed, 0, mcRows - 1);

    for (int wY = 0; wY < mcRows; wY++)
    {
        for (int wX = 0; wX < mcCols; wX++)
        {
            uint16_t wDx = abs(wX - wX1), wDy = abs(wY - wY1);
            uint16_t wDist = 2 * sqrt16((wDx * wDx) + (wDy * wDy));
            wDx = abs(wX - wX2); wDy = abs(wY - wY2);
            wDist += sqrt16((wDx * wDx) + (wDy * wDy));
            wDx = abs(wX - wX3); wDy = abs(wY - wY3);
            wDist += sqrt16((wDx * wDx) + (wDy * wDy));

            const uint8_t wColor = wDist ? static_cast<uint8_t>(1000 / wDist) : 255;
            if ((wColor > 0) && (wColor < 60)) f.SetXY(wX, wY, f.Pal(map(wColor * 9, 9, 531, 0, 255)));
            else                               f.SetXY(wX, wY, f.Pal(0));
        }
    }
    f.SetXY(wX1, wY1, CRGB::White);
    f.SetXY(wX2, wY2, CRGB::White);
    f.SetXY(wX3, wY3, CRGB::White);
}

/* Noise2D - by Andrew Tuline */
static void FxNoise2D(Fx& f)
{
    const uint16_t wScale = f.mIntensity + 2;
    for (int wY = 0; wY < mcRows; wY++)
    {
        for (int wX = 0; wX < mcCols; wX++)
        {
            f.SetXY(wX, wY, f.Pal(inoise8(wX * wScale, wY * wScale, f.mNow / (16 - f.mSpeed / 16))));
        }
    }
}

/* Octopus - by Stepko and Sutaburosu, adapted by blazoncek, RadialWave by softhack007 */
struct tOctoMap { uint8_t mAngle; uint8_t mRadius; };

static void FxOctopus(Fx& f)
{
    tOctoMap* wMap  = reinterpret_cast<tOctoMap*>(f.mData);
    uint8_t*  wOffs = f.mData + sizeof(tOctoMap) * mcCols * mcRows;
    const int wMapp = max(1, 180 / max(mcCols, mcRows));

    if ((f.mCall == 0) || (f.mCustom1 != wOffs[0]) || (f.mCustom2 != wOffs[1]))
    {
        wOffs[0] = f.mCustom1;
        wOffs[1] = f.mCustom2;
        const int wCx = mcCols / 2 + (f.mCustom1 - 128) * mcCols / 255;
        const int wCy = mcRows / 2 + (f.mCustom2 - 128) * mcRows / 255;
        for (int wX = 0; wX < mcCols; wX++)
        {
            for (int wY = 0; wY < mcRows; wY++)
            {
                const int wDx = wX - wCx, wDy = wY - wCy;
                wMap[Fx::XY(wX, wY)].mAngle  = static_cast<uint8_t>(int(40.7436f * atan2f(wDy, wDx)));
                wMap[Fx::XY(wX, wY)].mRadius = static_cast<uint8_t>(sqrtf(wDx * wDx + wDy * wDy) * wMapp);
            }
        }
    }

    f.mStep = (f.mNow * (f.mSpeed + 15)) / 33 / 25;
    const uint32_t wColorSpeed = f.mStep / 2;
    const uint32_t wOctoSpeed  = f.mCheck3 ? (4 * f.mStep / 5) : (f.mStep / 2);
    const int      wLegs       = f.mCustom3 / 4 + 1;

    for (int wX = 0; wX < mcCols; wX++)
    {
        for (int wY = 0; wY < mcRows; wY++)
        {
            const uint8_t wAngle  = wMap[Fx::XY(wX, wY)].mAngle;
            const uint8_t wRadius = wMap[Fx::XY(wX, wY)].mRadius;
            uint16_t wInt;
            if (f.mCheck3)
                wInt = sin8(static_cast<uint8_t>(wOctoSpeed + sin8(static_cast<uint8_t>(wOctoSpeed - wRadius)) + wAngle * wLegs));
            else
                wInt = sin8(static_cast<uint8_t>(sin8(static_cast<uint8_t>((wAngle * 4 - wRadius) / 4 + wOctoSpeed)) + wRadius - f.mStep + wAngle * wLegs));
            wInt = (wInt * wInt) / 255;
            f.SetXY(wX, wY, f.Pal(static_cast<uint8_t>(wColorSpeed - wRadius), wInt));
        }
    }
}

/* Plasma Ball - by Stepko, modified by Andrew Tuline */
static void FxPlasmaBall(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);
    f.FadeAll(f.mCustom1 >> 2);

    const uint32_t wT = (f.mNow * 8) / (256 - f.mSpeed);
    for (int wI = 0; wI < mcCols; wI++)
    {
        const uint8_t wVal = inoise8(wI * 30, wT, wT);
        const int     wMax = map(wVal, 0, 255, 0, mcCols - 1);
        for (int wJ = 0; wJ < mcRows; wJ++)
        {
            const int wMax2 = map(inoise8(wT, wJ * 30, wT), 0, 255, 0, mcRows - 1);
            const int wX  = wI + wMax2 - mcCols / 2;
            const int wY  = wJ + wMax  - mcCols / 2;
            const int wCx = wI + wMax2;
            const int wCy = wJ + wMax;
            const bool wOn = (((wX - wY) > -2) && ((wX - wY) < 2)) ||
                             (((mcCols - 1 - wX - wY) > -2) && ((mcCols - 1 - wX - wY) < 2)) ||
                             (mcCols - wCx == 0) || (mcCols - 1 - wCx == 0) ||
                             (mcRows - wCy == 0) || (mcRows - 1 - wCy == 0);
            f.AddXY(wI, wJ, wOn ? f.Pal(beat8(5), wVal) : CRGB(CRGB::Black));
        }
    }
    f.Blur(f.mCustom2 >> 5);
}

/* Polar Lights - by Kostyantyn Matviyevskyy, modified by Andrew Tuline */
static void FxPolarLights(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);

    const CRGBPalette16& wPal = ((f.mPaletteIndex != 0) || f.mCheck1) ? f.mPalette : mcAuroraPalette;
    const float wAdjustHeight = constrain(28.0f + float(mcRows - 8) * (12.0f - 28.0f) / 24.0f, 12.0f, 28.0f);
    const int   wAdjScale     = constrain(310 + (mcCols - 8) * (63 - 310) / 24, 63, 310);
    const uint16_t wScale     = map(f.mIntensity, 0, 255, 30, wAdjScale);
    const uint8_t  wSpeed     = map(f.mSpeed, 0, 255, 136, 20);
    const float    wRows2     = float(mcRows) / 2.0f;

    f.mStep = (f.mNow * (mcCols * mcRows)) / 25;
    for (int wX = 0; wX < mcCols; wX++)
    {
        for (int wY = 0; wY < mcRows; wY++)
        {
            f.mStep++;
            const uint8_t wNoise = inoise8((f.mStep % 2) + wX * wScale, wY * 16 + f.mStep % 16, f.mStep / wSpeed);
            f.SetXY(wX, wY, ColorFromPalette(wPal, qsub8(wNoise, static_cast<uint8_t>(fabsf(wRows2 - float(wY)) * wAdjustHeight)), 255, LINEARBLEND));
        }
    }
    if (f.mCheck3) f.BlurRows(192);
}

/* Pulser - by ldirko, modified by Andrew Tuline */
static void FxPulser(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);
    f.FadeAll(8 - (f.mIntensity >> 5));

    const uint32_t wA = f.mNow / (18 - f.mSpeed / 16);
    const int wX = (wA / 14) % mcCols;
    const int wY = map(sin8(wA * 5) + sin8(wA * 4) + sin8(wA * 2), 0, 765, mcRows - 1, 0);
    f.SetXY(wX, wY, f.Pal(map(wY, 0, mcRows - 1, 0, 255)));
    f.Blur(1 + (f.mIntensity >> 4));
}

/* Sindots - by ldirko, modified by Andrew Tuline */
static void FxSindots(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);
    f.FadeAll(f.mCustom1 >> 3);

    const uint8_t wT1 = f.mNow / (257 - f.mSpeed);
    const uint8_t wT2 = sin8(wT1) / 4 * 2;
    for (int wI = 0; wI < 13; wI++)
    {
        const int wX = sin8(wT1 + wI * f.mIntensity / 8) * (mcCols - 1) / 255;
        const int wY = sin8(wT2 + wI * f.mIntensity / 8) * (mcRows - 1) / 255;
        f.SetXY(wX, wY, f.Pal(wI * 255 / 13));
    }
    f.Blur(f.mCustom2 >> 3);
}

/* Squared Swirl - by Mark Kriegsman, modified by Andrew Tuline */
static void FxSquaredSwirl(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);

    constexpr uint8_t wBorder = 2;
    f.FadeAll(24);
    f.Blur(f.mCustom3 << 3);

    const uint8_t wI = beatsin8(19, wBorder, mcCols - wBorder);
    const uint8_t wJ = beatsin8(22, wBorder, mcCols - wBorder);
    const uint8_t wK = beatsin8(17, wBorder, mcCols - wBorder);
    const uint8_t wM = beatsin8(18, wBorder, mcRows - wBorder);
    const uint8_t wN = beatsin8(15, wBorder, mcRows - wBorder);
    const uint8_t wP = beatsin8(20, wBorder, mcRows - wBorder);
    const uint16_t wMs = f.mNow;

    f.AddXY(wI, wM, f.Pal(wMs / 29));
    f.AddXY(wJ, wN, f.Pal(wMs / 41));
    f.AddXY(wK, wP, f.Pal(wMs / 73));
}

/* Sun Radiation - by ldirko, modified by Andrew Tuline */
static void FxSunRadiation(Fx& f)
{
    uint8_t* wBump = f.mData;   // (cols+2) * (rows+2) bytes
    if (f.mCall == 0) f.Fill(CRGB::Black);

    const uint32_t wT = f.mNow / 4;
    const uint8_t  wSomeVal = f.mSpeed / 4;
    int wIndex = 0;
    for (int wJ = 0; wJ < (mcRows + 2); wJ++)
    {
        for (int wI = 0; wI < (mcCols + 2); wI++)
        {
            wBump[wIndex++] = static_cast<uint8_t>((int16_t(inoise8(wI * wSomeVal, wJ * wSomeVal, wT)) - 0x7F) / 3);
        }
    }

    int wYIndex = mcCols + 3;
    int16_t wVly = -(mcRows / 2 + 1);
    for (int wY = 0; wY < mcRows; wY++)
    {
        ++wVly;
        int16_t wVlx = -(mcCols / 2 + 1);
        for (int wX = 0; wX < mcCols; wX++)
        {
            ++wVlx;
            const int8_t wNx = wBump[wX + wYIndex + 1] - wBump[wX + wYIndex - 1];
            const int8_t wNy = wBump[wX + wYIndex + (mcCols + 2)] - wBump[wX + wYIndex - (mcCols + 2)];
            const int wDifX = min(abs(wVlx * 7 - wNx), 255);
            const int wDifY = min(abs(wVly * 7 - wNy), 255);
            int wCol = 255 - (wDifX * wDifX + wDifY * wDifY) / 8;
            if (wCol < 0) wCol = 0;
            f.SetXY(wX, wY, HeatColor(static_cast<uint8_t>(wCol / (3.0f - float(f.mIntensity) / 128.f))));
        }
        wYIndex += (mcCols + 2);
    }
}

/* Tartan - by Elliott Kember, modified by Andrew Tuline */
static void FxTartan(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);

    const int wOffsetX = int(beatsin16(3, 0, 720)) - 360;
    const int wOffsetY = int(beatsin16(2, 0, 720)) - 360;
    const int wSharpness = f.mCustom3 / 8;

    for (int wX = 0; wX < mcCols; wX++)
    {
        for (int wY = 0; wY < mcRows; wY++)
        {
            uint8_t  wHue = wX * beatsin16(10, 1, 10) + wOffsetY;
            uint8_t  wBri = sin8(wX * f.mSpeed / 2 + wOffsetX);
            uint32_t wInt = wBri;
            for (int wI = 0; wI < wSharpness; wI++) wInt *= wBri;
            wInt >>= 8 * wSharpness;
            f.SetXY(wX, wY, f.Pal(wHue, wInt));

            wHue = wY * 3 + wOffsetX;
            wBri = sin8(wY * f.mIntensity / 2 + wOffsetY);
            wInt = wBri;
            for (int wI = 0; wI < wSharpness; wI++) wInt *= wBri;
            wInt >>= 8 * wSharpness;
            f.AddXY(wX, wY, f.Pal(wHue, wInt));
        }
    }
}

/* Spaceships - by Stepko, adapted by blazoncek */
static void FxSpaceships(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);

    const uint32_t wTb = f.mNow >> 12;   // every ~4s
    if (wTb > f.mStep)
    {
        int wDir = ++f.mAux0;
        wDir += int(random8(3)) - 1;
        if      (wDir > 7) f.mAux0 = 0;
        else if (wDir < 0) f.mAux0 = 7;
        else               f.mAux0 = wDir;
        f.mStep = wTb + random8(4);
    }

    f.FadeAll(map(f.mSpeed, 0, 255, 248, 16));
    f.Move(f.mAux0, 1);

    for (int wI = 0; wI < 8; wI++)
    {
        const int wX = beatsin8(12 + wI, 2, mcCols - 3);
        const int wY = beatsin8(15 + wI, 2, mcRows - 3);
        f.AddXY(wX, wY, f.Pal(beatsin8(12 + wI, 0, 255)));
    }
    f.Blur(f.mIntensity >> 3);
}

/* Crazy Bees - by Stepko, adapted by blazoncek */
struct tBee
{
    uint8_t mPosX, mPosY, mAimX, mAimY, mHue;
    int8_t  mSignX, mSignY;
    int16_t mDeltaX, mDeltaY, mError;

    void Aimed(int aW, int aH)
    {
        mAimX   = random8(0, aW);
        mAimY   = random8(0, aH);
        mHue    = random8();
        mDeltaX = abs(mAimX - mPosX);
        mDeltaY = abs(mAimY - mPosY);
        mSignX  = (mPosX < mAimX) ? 1 : -1;
        mSignY  = (mPosY < mAimY) ? 1 : -1;
        mError  = mDeltaX - mDeltaY;
    }
};

static void FxCrazyBees(Fx& f)
{
    tBee* wBees = reinterpret_cast<tBee*>(f.mData);
    const int wN = min(5, (mcRows * mcCols) / 256 + 1);

    if (f.mCall == 0)
    {
        f.Fill(CRGB::Black);
        for (int wI = 0; wI < wN; wI++)
        {
            wBees[wI].mPosX = random8(0, mcCols);
            wBees[wI].mPosY = random8(0, mcRows);
            wBees[wI].Aimed(mcCols, mcRows);
        }
    }

    if (f.mNow <= f.mStep) return;
    f.mStep = f.mNow + (mcFrameTime * 8 / ((f.mSpeed >> 5) + 1));

    f.FadeAll(32);
    for (int wI = 0; wI < wN; wI++)
    {
        tBee& wB = wBees[wI];
        const CRGB wAim = CHSV(wB.mHue, 255, 255);
        f.AddXY(wB.mAimX + 1, wB.mAimY, wAim);
        f.AddXY(wB.mAimX, wB.mAimY + 1, wAim);
        f.AddXY(wB.mAimX - 1, wB.mAimY, wAim);
        f.AddXY(wB.mAimX, wB.mAimY - 1, wAim);

        if ((wB.mPosX != wB.mAimX) || (wB.mPosY != wB.mAimY))
        {
            f.SetXY(wB.mPosX, wB.mPosY, CHSV(wB.mHue, 60, 255));
            const int wError2 = wB.mError * 2;
            if (wError2 > -wB.mDeltaY) { wB.mError -= wB.mDeltaY; wB.mPosX += wB.mSignX; }
            if (wError2 <  wB.mDeltaX) { wB.mError += wB.mDeltaX; wB.mPosY += wB.mSignY; }
        }
        else
        {
            wB.Aimed(mcCols, mcRows);
        }
    }
    f.Blur(f.mIntensity >> 4);
}

/* Ghost Rider - by Stepko, adapted by blazoncek */
static constexpr int mcLighters = 32;
struct tGhost
{
    int16_t  mPosX, mPosY;
    uint16_t mAngle;
    int8_t   mAngleSpeed, mVSpeed;
    uint16_t mLPosX[mcLighters], mLPosY[mcLighters], mLAngle[mcLighters], mLTime[mcLighters];
    bool     mLReg[mcLighters];
};

static void FxGhostRider(Fx& f)
{
    tGhost* wG = reinterpret_cast<tGhost*>(f.mData);
    const int wMax = min(mcCols + mcRows, mcLighters);

    if (f.mCall == 0)
    {
        f.Fill(CRGB::Black);
        wG->mAngleSpeed = int(random8(0, 20)) - 10;
        wG->mVSpeed = 5;
        wG->mPosX = (mcCols / 2) * 10;
        wG->mPosY = (mcRows / 2) * 10;
        wG->mAngle = 0;
        for (int wI = 0; wI < wMax; wI++)
        {
            wG->mLPosX[wI] = wG->mPosX;
            wG->mLPosY[wI] = wG->mPosY + wI;
            wG->mLTime[wI] = wI * 2;
            wG->mLAngle[wI] = 0;
            wG->mLReg[wI] = false;
        }
    }

    if (f.mNow <= f.mStep) return;
    f.mStep = f.mNow + 1024 / (mcCols + mcRows);

    f.FadeAll((f.mSpeed >> 2) + 64);
    f.WuPixel(wG->mPosX * 256 / 10, wG->mPosY * 256 / 10, CRGB::White);

    wG->mPosX = wG->mPosX + wG->mVSpeed * sinf(radians(wG->mAngle));
    wG->mPosY = wG->mPosY + wG->mVSpeed * cosf(radians(wG->mAngle));
    wG->mAngle += wG->mAngleSpeed;
    if (wG->mPosX < 0)                 wG->mPosX = (mcCols - 1) * 10;
    if (wG->mPosX > (mcCols - 1) * 10) wG->mPosX = 0;
    if (wG->mPosY < 0)                 wG->mPosY = (mcRows - 1) * 10;
    if (wG->mPosY > (mcRows - 1) * 10) wG->mPosY = 0;

    for (int wI = 0; wI < wMax; wI++)
    {
        wG->mLTime[wI] += random8(5, 20);
        if ((wG->mLTime[wI] >= 255) ||
            (wG->mLPosX[wI] <= 0) || (wG->mLPosX[wI] >= (mcCols - 1) * 10) ||
            (wG->mLPosY[wI] <= 0) || (wG->mLPosY[wI] >= (mcRows - 1) * 10))
        {
            wG->mLReg[wI] = true;
        }
        if (wG->mLReg[wI])
        {
            wG->mLPosY[wI]  = wG->mPosY;
            wG->mLPosX[wI]  = wG->mPosX;
            wG->mLAngle[wI] = wG->mAngle + (int(random8(20)) - 10);
            wG->mLTime[wI]  = 0;
            wG->mLReg[wI]   = false;
        }
        else
        {
            wG->mLPosX[wI] = static_cast<uint16_t>(static_cast<int>(wG->mLPosX[wI] - 7 * sinf(radians(wG->mLAngle[wI]))));
            wG->mLPosY[wI] = static_cast<uint16_t>(static_cast<int>(wG->mLPosY[wI] - 7 * cosf(radians(wG->mLAngle[wI]))));
        }
        f.WuPixel(wG->mLPosX[wI] * 256 / 10, wG->mLPosY[wI] * 256 / 10, f.Pal(static_cast<uint8_t>(256 - wG->mLTime[wI])));
    }
    f.Blur(f.mIntensity >> 3);
}

/* Floating Blobs - by Stepko, adapted by blazoncek */
static constexpr int mcMaxBlobs = 8;
struct tBlobs
{
    float   mX[mcMaxBlobs], mY[mcMaxBlobs], mSX[mcMaxBlobs], mSY[mcMaxBlobs], mR[mcMaxBlobs];
    bool    mGrow[mcMaxBlobs];
    uint8_t mColor[mcMaxBlobs];
};

static void FxBlobs(Fx& f)
{
    tBlobs* wB = reinterpret_cast<tBlobs*>(f.mData);
    const uint8_t wAmount = (f.mIntensity >> 5) + 1;

    if (f.mCall == 0)
    {
        f.Fill(CRGB::Black);
        f.mStep = 0;
        for (int wI = 0; wI < mcMaxBlobs; wI++)
        {
            wB->mR[wI]  = random8(1, (mcCols > 8) ? (mcCols / 4) : 2);
            wB->mSX[wI] = float(random8(3, mcCols)) / float(256 - f.mSpeed);
            wB->mSY[wI] = float(random8(3, mcRows)) / float(256 - f.mSpeed);
            wB->mX[wI]  = random8(0, mcCols - 1);
            wB->mY[wI]  = random8(0, mcRows - 1);
            wB->mColor[wI] = random8();
            wB->mGrow[wI]  = (wB->mR[wI] < 1.f);
        }
    }

    f.FadeAll(20);
    const float wMaxGrow = min(min(mcCols, mcRows) / 4.f, 2.f);

    for (int wI = 0; wI < wAmount; wI++)
    {
        if (f.mStep < f.mNow) wB->mColor[wI] = add8(wB->mColor[wI], 4);

        const float wDelta = max(fabsf(wB->mSX[wI]), fabsf(wB->mSY[wI])) * 0.05f;
        if (wB->mGrow[wI]) { wB->mR[wI] += wDelta; if (wB->mR[wI] >= wMaxGrow) wB->mGrow[wI] = false; }
        else               { wB->mR[wI] -= wDelta; if (wB->mR[wI] < 0.8f)      wB->mGrow[wI] = true;  }

        const CRGB wC = f.Pal(wB->mColor[wI]);
        if (wB->mR[wI] > 1.f) f.FillCircle(roundf(wB->mX[wI]), roundf(wB->mY[wI]), roundf(wB->mR[wI]), wC);
        else                  f.SetXY(roundf(wB->mX[wI]), roundf(wB->mY[wI]), wC);

        /* Move */
        if      (wB->mX[wI] + wB->mR[wI] >= mcCols - 1) wB->mX[wI] += wB->mSX[wI] * ((mcCols - 1 - wB->mX[wI]) / wB->mR[wI] + 0.005f);
        else if (wB->mX[wI] - wB->mR[wI] <= 0)          wB->mX[wI] += wB->mSX[wI] * (wB->mX[wI] / wB->mR[wI] + 0.005f);
        else                                            wB->mX[wI] += wB->mSX[wI];
        if      (wB->mY[wI] + wB->mR[wI] >= mcRows - 1) wB->mY[wI] += wB->mSY[wI] * ((mcRows - 1 - wB->mY[wI]) / wB->mR[wI] + 0.005f);
        else if (wB->mY[wI] - wB->mR[wI] <= 0)          wB->mY[wI] += wB->mSY[wI] * (wB->mY[wI] / wB->mR[wI] + 0.005f);
        else                                            wB->mY[wI] += wB->mSY[wI];

        /* Bounce */
        if (wB->mX[wI] < 0.01f)
        {
            wB->mSX[wI] = float(random8(3, mcCols)) / (256 - f.mSpeed);
            wB->mX[wI]  = 0.01f;
        }
        else if (wB->mX[wI] > float(mcCols) - 1.01f)
        {
            wB->mSX[wI] = -float(random8(3, mcCols)) / (256 - f.mSpeed);
            wB->mX[wI]  = float(mcCols) - 1.01f;
        }
        if (wB->mY[wI] < 0.01f)
        {
            wB->mSY[wI] = float(random8(3, mcRows)) / (256 - f.mSpeed);
            wB->mY[wI]  = 0.01f;
        }
        else if (wB->mY[wI] > float(mcRows) - 1.01f)
        {
            wB->mSY[wI] = -float(random8(3, mcRows)) / (256 - f.mSpeed);
            wB->mY[wI]  = float(mcRows) - 1.01f;
        }
    }
    f.Blur(f.mCustom1 >> 2);

    if (f.mStep < f.mNow) f.mStep = f.mNow + 2000;   // change colors every 2 seconds
}

/* Scrolling Text - based on the WLED effect, shows the current time */
static void FxScrollingText(Fx& f)
{
    const int wCharW = Font5x7NS::mGlyphWidth + Font5x7NS::mGlyphSpacing;
    const int wLen   = strlen(f.mText);
    const int wTextW = wLen * wCharW;
    const int wYOff  = map(f.mIntensity, 0, 255, -mcRows / 2, mcRows / 2) + (mcRows - Font5x7NS::mGlyphHeight) / 2;

    if (f.mCall == 0) { f.Fill(CRGB::Black); f.mAux0 = 0; f.mStep = 0; }

    if (static_cast<int32_t>(f.mNow - f.mStep) >= 0)
    {
        if (wTextW > mcCols) f.mAux0 = (f.mAux0 + 1) % (wTextW + mcCols);
        else                 f.mAux0 = (mcCols + wTextW) / 2;
        f.mAux1 = (f.mAux1 + 1) & 0xFF;
        f.mStep = f.mNow + map(f.mSpeed, 0, 255, 250, 30);
    }

    f.FadeAll(255 - (f.mCustom1 >> 1));
    const CRGB wColor = f.Pal(f.mAux1);

    for (int wI = 0; wI < wLen; wI++)
    {
        const int wX0 = mcCols - int(f.mAux0) + wCharW * wI;
        if ((wX0 + wCharW < 0) || (wX0 >= mcCols)) continue;

        const uint8_t* wGlyph = Font5x7NS::GetGlyph(f.mText[wI]);
        for (int wCol = 0; wCol < Font5x7NS::mGlyphWidth; wCol++)
        {
            for (int wRow = 0; wRow < Font5x7NS::mGlyphHeight; wRow++)
            {
                if (wGlyph[wCol] & (1U << wRow)) f.SetXY(wX0 + wCol, wYOff + wRow, wColor);
            }
        }
    }
}

/* Drift Rose - by Stepko, adapted by blazoncek */
static void FxDriftRose(Fx& f)
{
    const float wCx = (mcCols - mcCols % 2) / 2.f - .5f;
    const float wCy = (mcRows - mcRows % 2) / 2.f - .5f;
    unsigned wL2 = f.mCheck3 ? max(mcCols, mcRows) : min(mcCols, mcRows);
    if (f.mCheck3 && (abs(mcCols - mcRows) < 4)) wL2 = wL2 * 1.4142f;
    const float wL = wL2 / 2.f;

    if (f.mCall == 0) f.Fill(CRGB::Black);
    f.FadeAll(32 + (f.mSpeed >> 3));

    for (int wI = 1; wI < 37; wI++)
    {
        const float    wAngle = float(DEG_TO_RAD) * wI * 10;
        const uint32_t wX = int((wCx + (sinf(wAngle) * (beatsin8(wI, 0, wL2) - wL))) * 255.f);
        const uint32_t wY = int((wCy + (cosf(wAngle) * (beatsin8(wI, 0, wL2) - wL))) * 255.f);
        if ((wX < uint32_t(mcCols * 256)) && (wY < uint32_t(mcRows * 256))) f.WuPixel(wX, wY, CHSV(wI * 10, 255, 255));
    }
    f.Blur((f.mIntensity >> 4) + 1);
}

/* Distortion Waves - by ldirko, adapted by blazoncek, improvements by dedehai */
static void FxDistortionWaves(Fx& f)
{
    if (f.mCall == 0) f.Fill(CRGB::Black);

    const uint8_t wSpeed = f.mSpeed / 32;
    uint8_t wScale = f.mIntensity / 32;
    if (f.mCheck2) wScale += 192 / (mcCols + mcRows);

    const uint32_t wA  = f.mNow / 32;
    const uint32_t wA2 = wA / 2;
    const uint32_t wA3 = wA / 3;
    const uint32_t wColsScaled = mcCols * wScale;
    const uint32_t wRowsScaled = mcRows * wScale;

    const uint32_t wCx  = beatsin16(10 - wSpeed, 0, wColsScaled);
    const uint32_t wCy  = beatsin16(12 - wSpeed, 0, wRowsScaled);
    const uint32_t wCx1 = beatsin16(13 - wSpeed, 0, wColsScaled);
    const uint32_t wCy1 = beatsin16(15 - wSpeed, 0, wRowsScaled);
    const uint32_t wCx2 = beatsin16(17 - wSpeed, 0, wColsScaled);
    const uint32_t wCy2 = beatsin16(14 - wSpeed, 0, wRowsScaled);
    const bool wUsePalette = (f.mPaletteIndex != 0);

    uint32_t wXOffs = 0;
    for (int wX = 0; wX < mcCols; wX++)
    {
        wXOffs += wScale;
        uint32_t wYOffs = 0;
        for (int wY = 0; wY < mcRows; wY++)
        {
            wYOffs += wScale;
            uint8_t wRd, wGd, wBd;
            if (f.mCheck3)
            {
                wRd = cos8(((wX + wY) * 8 + wA2) & 255) >> 1;
                wGd = cos8(((wX + wY) * 8 + wA3 + 32) & 255) >> 1;
                wBd = cos8(((wX + wY) * 8 + wA + 64) & 255) >> 1;
            }
            else
            {
                wRd = cos8((cos8(((wX << 3) + wA ) & 255) + cos8(((wY << 3) - wA2) & 255) + wA3     ) & 255) >> 1;
                wGd = cos8((cos8(((wX << 3) - wA2) & 255) + cos8(((wY << 3) + wA3) & 255) + wA + 32 ) & 255) >> 1;
                wBd = cos8((cos8(((wX << 3) + wA3) & 255) + cos8(((wY << 3) - wA ) & 255) + wA2 + 64) & 255) >> 1;
            }

            uint8_t wR = wRd + ((wA  - (((wXOffs - wCx)  * (wXOffs - wCx)  + (wYOffs - wCy)  * (wYOffs - wCy))  >> 7)) << 1);
            uint8_t wG = wGd + ((wA2 - (((wXOffs - wCx1) * (wXOffs - wCx1) + (wYOffs - wCy1) * (wYOffs - wCy1)) >> 7)) << 1);
            uint8_t wB = wBd + ((wA3 - (((wXOffs - wCx2) * (wXOffs - wCx2) + (wYOffs - wCy2) * (wYOffs - wCy2)) >> 7)) << 1);

            wR = dim8_raw(cos8(wR));
            wG = dim8_raw(cos8(wG));
            wB = dim8_raw(cos8(wB));

            if (!wUsePalette)
            {
                f.SetXY(wX, wY, CRGB(wR, wG, wB));
            }
            else
            {
                const uint8_t wBrightness = (wR + wG + wB) / 3;
                if (f.mCheck1)
                {
                    f.SetXY(wX, wY, f.Pal(wBrightness));
                }
                else
                {
                    const CHSV wHsv = rgb2hsv_approximate(CRGB(wR >> 2, wG >> 2, wB >> 2));
                    f.SetXY(wX, wY, f.Pal(wHsv.h, wBrightness));
                }
            }
        }
    }
    if (!f.mCheck1 && wUsePalette) f.Blur(200);
}

/* Soap - by Stepko, adapted by blazoncek, optimization by dedehai */
static constexpr size_t mcSoapNoiseOffset  = 0;
static constexpr size_t mcSoapPixelsOffset = mcCols * mcRows;
static constexpr size_t mcSoapCoordOffset  = mcSoapPixelsOffset + mcCols * mcRows * sizeof(CRGB);

static void SoapPixels(Fx& f, bool aIsRow, uint8_t* apNoise, CRGB* apPixels)
{
    const int wTRC = aIsRow ? mcRows : mcCols;
    const int wTCR = aIsRow ? mcCols : mcRows;
    const int wAmplitude = max(1, (wTCR - 8) >> 3) * (1 + (f.mCustom1 >> 5));
    CRGB wBuff[max(mcCols, mcRows)];

    for (int wI = 0; wI < wTRC; wI++)
    {
        const int wAmount   = (int(apNoise[aIsRow ? wI * mcCols : wI]) - 128) * wAmplitude;
        const int wDelta    = abs(wAmount) >> 8;
        const int wFraction = abs(wAmount) & 255;

        for (int wJ = 0; wJ < wTCR; wJ++)
        {
            int wZD, wZF;
            if (wAmount < 0) { wZD = wJ - wDelta; wZF = wZD - 1; }
            else             { wZD = wJ + wDelta; wZF = wZD + 1; }
            int wYA = abs(wZD) % wTCR, wYB = abs(wZF) % wTCR;
            int wXA = wI, wXB = wI;
            if (aIsRow) { std::swap(wXA, wYA); std::swap(wXB, wYB); }
            const int wIdxA = Fx::XY(wXA, wYA);
            const int wIdxB = Fx::XY(wXB, wYB);

            CRGB wPixelA = ((wZD >= 0) && (wZD < wTCR)) ? apPixels[wIdxA] : f.Pal(static_cast<uint8_t>(~apNoise[wIdxA] * 3));
            CRGB wPixelB = ((wZF >= 0) && (wZF < wTCR)) ? apPixels[wIdxB] : f.Pal(static_cast<uint8_t>(~apNoise[wIdxB] * 3));
            wBuff[wJ] = wPixelA.nscale8(ease8InOutApprox(255 - wFraction)) + wPixelB.nscale8(ease8InOutApprox(wFraction));
        }

        for (int wJ = 0; wJ < wTCR; wJ++)
        {
            if (aIsRow) apPixels[Fx::XY(wJ, wI)] = wBuff[wJ];
            else        apPixels[Fx::XY(wI, wJ)] = wBuff[wJ];
        }
    }
}

static void FxSoap(Fx& f)
{
    uint8_t*  wNoise  = f.mData + mcSoapNoiseOffset;
    CRGB*     wPixels = reinterpret_cast<CRGB*>(f.mData + mcSoapPixelsOffset);
    uint32_t* wCoord  = reinterpret_cast<uint32_t*>(f.mData + mcSoapCoordOffset);

    const uint32_t wScaleX = 160000U / mcCols;
    const uint32_t wScaleY = 160000U / mcRows;
    const uint32_t wMov    = min(mcCols, mcRows) * (f.mSpeed + 2) / 2;
    const uint8_t  wSmooth = min(250, int(f.mIntensity));

    if (f.mCall == 0) for (int wI = 0; wI < 3; wI++) wCoord[wI] = esp_random();
    else              for (int wI = 0; wI < 3; wI++) wCoord[wI] += wMov;

    for (int wI = 0; wI < mcCols; wI++)
    {
        const int32_t wIOff = wScaleX * (wI - mcCols / 2);
        for (int wJ = 0; wJ < mcRows; wJ++)
        {
            const int32_t wJOff = wScaleY * (wJ - mcRows / 2);
            const uint8_t wData = inoise16(wCoord[0] + wIOff, wCoord[1] + wJOff, wCoord[2]) >> 8;
            const int wIdx = Fx::XY(wI, wJ);
            wNoise[wIdx] = scale8(wNoise[wIdx], wSmooth) + scale8(wData, 255 - wSmooth);
        }
    }

    if (f.mCall == 0)
    {
        for (int wI = 0; wI < mcCols * mcRows; wI++) wPixels[wI] = f.Pal(static_cast<uint8_t>(~wNoise[wI] * 3));
    }

    SoapPixels(f, true,  wNoise, wPixels);
    SoapPixels(f, false, wNoise, wPixels);
    memcpy(f.mLeds, wPixels, sizeof(f.mLeds));
}

/* Waving Cell - by Stepko, adapted by blazoncek, improvements by dedehai */
static void FxWavingCell(Fx& f)
{
    const uint32_t wT  = (f.mNow * (f.mSpeed + 1)) >> 3;
    const uint32_t wAX = f.mCustom1 / 16 + 9;
    const uint32_t wAY = f.mCustom2 / 16 + 1;
    const uint32_t wAZ = f.mCustom3 + 1;

    for (int wX = 0; wX < mcCols; wX++)
    {
        for (int wY = 0; wY < mcRows; wY++)
        {
            const uint32_t wWave = sin8((wX * wAX) + sin8(((((uint32_t)wY << 8) + wT) * wAY) >> 8)) + cos8(wY * wAZ);
            f.SetXY(wX, wY, f.Pal(wWave + (wT >> (8 - (f.mCheck2 * 3)))));
        }
    }
    f.Blur(f.mIntensity);
}

/* Snow Fall - by Brandon Butler */
static bool SnowGet(const uint8_t* apGrid, int aN)          { return (apGrid[aN >> 3] >> (aN & 7)) & 1; }
static void SnowSet(uint8_t* apGrid, int aN, bool aValue)
{
    if (aValue) apGrid[aN >> 3] |=  (1 << (aN & 7));
    else        apGrid[aN >> 3] &= ~(1 << (aN & 7));
}

static void FxSnowFall(Fx& f)
{
    uint8_t* wGrid = f.mData;
    const CRGB wBg = CRGB::Black;

    if (f.mCall == 0)
    {
        f.Fill(wBg);
        f.mAux0 = 0;
        f.mStep = 0;
        memset(wGrid, 0, (mcCols * mcRows + 7) / 8);
    }

    const uint8_t wSpeed = map(f.mSpeed, 0, 255, 0, 60);
    if (!wSpeed || ((f.mNow - f.mStep) < (1000U / wSpeed))) return;

    const uint8_t wBlur = map(f.mCustom2, 0, 255, 255, 0);
    const uint8_t wSway = f.mCustom3;

    /* Despawn snow on the bottom row */
    const bool wOverflow = f.mAux0 && f.mCheck3;
    const int  wDespawn  = (f.mCustom1 == 255) ? 256 : map(f.mCustom1, 0, 255, 0, 100);
    const int  wLastY    = mcRows - 1;
    for (int wX = 0; wX < mcCols; wX++)
    {
        if (wOverflow || (random8() < wDespawn)) SnowSet(wGrid, wLastY * mcCols + wX, false);
        if (SnowGet(wGrid, wLastY * mcCols + wX)) continue;
        f.BlendXY(wX, wLastY, wBg, wBlur);
    }
    if (f.mAux0) --f.mAux0;

    /* Shuffled column order randomizes the movement */
    uint8_t wShuffle[mcCols];
    for (int wI = 0; wI < mcCols; wI++) wShuffle[wI] = wI;
    for (int wI = mcCols - 1; wI > 0; wI--) std::swap(wShuffle[wI], wShuffle[random8(wI + 1)]);

    for (int wY = mcRows - 2; wY >= 0; wY--)
    {
        for (int wI = 0; wI < mcCols; wI++)
        {
            const int  wX     = wShuffle[wI];
            const int  wPos   = Fx::XY(wX, wY);
            const CRGB wColor = f.GetXY(wX, wY);

            if (!SnowGet(wGrid, wPos))
            {
                if (wBlur) f.SetXY(wX, wY, blend(wColor, wBg, wBlur));
                continue;
            }

            int wNewX = wX, wNewY = wY + 1;
            const int  wNewPos    = Fx::XY(wNewX, wNewY);
            const bool wDown      = !SnowGet(wGrid, wNewPos);
            const bool wDownLeft  = (wX > 0)          && !SnowGet(wGrid, wNewPos - 1);
            const bool wDownRight = (wX < mcCols - 1) && !SnowGet(wGrid, wNewPos + 1);

            if (!wDown)
            {
                if (wDownLeft && wDownRight) wNewX = random8(2) ? wX - 1 : wX + 1;
                else if (wDownLeft)          wNewX = wX - 1;
                else if (wDownRight)         wNewX = wX + 1;
                else                         wNewY = wY;
            }
            else if (wSway && (random8(30) < wSway))
            {
                if      ((wX % 2 == 1) && wDownLeft  && !SnowGet(wGrid, wPos - 1)) wNewX = wX - 1;
                else if ((wX % 2 == 0) && wDownRight && !SnowGet(wGrid, wPos + 1)) wNewX = wX + 1;
            }

            if ((wNewY != wY) || (wNewX != wX))
            {
                SnowSet(wGrid, wPos, false);
                SnowSet(wGrid, Fx::XY(wNewX, wNewY), true);
                f.SetXY(wX, wY, blend(wColor, wBg, wBlur));
            }
            f.SetXY(wNewX, wNewY, wColor);
        }
    }

    /* Spawn snow on the top row */
    const int wSpawn = map(f.mIntensity, 0, 255, 0, 100);
    for (int wX = 0; wX < mcCols; wX++)
    {
        if (random8() >= wSpawn) continue;
        if (SnowGet(wGrid, wX)) { f.mAux0 = mcRows; continue; }

        SnowSet(wGrid, wX, true);
        if (f.mCheck1) f.SetXY(wX, 0, f.Pal(random8()));
        else           { const uint8_t wC = random8(120, 200); f.SetXY(wX, 0, CRGB(wC, wC, wC)); }
    }

    f.mStep = f.mNow;
}


/***************************************************************************************************
  Effect table
 **************************************************************************************************/
/* Default custom values and options as in WLED-MM */
static const tEffectInfo mcEffects2D[] =
{
    { "Black Hole",       FxBlackHole,       TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Blobs",            FxBlobs,           TYPE_2D, MAP_PIXELS, 0,   8, 128, 16, false, false, false },
    { "Colored Bursts",   FxColoredBursts,   TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Crazy Bees",       FxCrazyBees,       TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Distortion Waves", FxDistortionWaves, TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "DNA",              FxDna,             TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "DNA Spiral",       FxDnaSpiral,       TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Drift",            FxDrift,           TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Drift Rose",       FxDriftRose,       TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Firenoise",        FxFirenoise,       TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Frizzles",         FxFrizzles,        TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Game Of Life",     FxGameOfLife,      TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, true  },
    { "Ghost Rider",      FxGhostRider,      TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Hiphotic",         FxHiphotic,        TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Julia",            FxJulia,           TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Lissajous",        FxLissajous,       TYPE_2D, MAP_PIXELS, 0, 128, 128, 15, false, false, false },
    { "Matrix",           FxMatrix,          TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Metaballs",        FxMetaballs,       TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Noise2D",          FxNoise2D,         TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Octopus",          FxOctopus,         TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Plasma Ball",      FxPlasmaBall,      TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Polar Lights",     FxPolarLights,     TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Pulser",           FxPulser,          TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Scrolling Text",   FxScrollingText,   TYPE_2D, MAP_PIXELS, 0,   0, 128, 16, false, false, false },
    { "Sindots",          FxSindots,         TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Snow Fall",        FxSnowFall,        TYPE_2D, MAP_PIXELS, 0,  17,   0,  0, false, false, true  },
    { "Soap",             FxSoap,            TYPE_2D, MAP_PIXELS, 1, 128, 128, 16, false, false, false },
    { "Spaceships",       FxSpaceships,      TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Squared Swirl",    FxSquaredSwirl,    TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Sun Radiation",    FxSunRadiation,    TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Tartan",           FxTartan,          TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
    { "Waving Cell",      FxWavingCell,      TYPE_2D, MAP_PIXELS, 0, 128, 128, 16, false, false, false },
};

static constexpr uint8_t mcEffects2DCount = sizeof(mcEffects2D) / sizeof(mcEffects2D[0]);

/* Maximum number of effects (2D + 1D) and name length including the 1D/2D mark */
static constexpr uint8_t mcMaxEffects = 200;
static constexpr size_t  mcNameLength = 32;

static const char* const mcPaletteNames[] =
{
    "Default", "Rainbow", "Party", "Ocean", "Forest", "Lava", "Heat", "Cloud", "Rainbow Stripes",
    "Colors 1&2", "Colors 1-3"
};

static constexpr uint8_t mcPaletteCount = sizeof(mcPaletteNames) / sizeof(mcPaletteNames[0]);

static const char* const mcMappingNames[] =
{
    "Effect default", "Pixels", "Bar", "Circle", "Corner"
};

static constexpr uint8_t mcMappingCount = sizeof(mcMappingNames) / sizeof(mcMappingNames[0]);

/* Per-effect state must fit into the data buffer */
static_assert(sizeof(tGhost)  <= mcDataSize, "Effects: data buffer too small (Ghost Rider)");
static_assert(sizeof(tBlobs)  <= mcDataSize, "Effects: data buffer too small (Blobs)");
static_assert(sizeof(tBee) * 5 <= mcDataSize, "Effects: data buffer too small (Crazy Bees)");
static_assert(mcSoapCoordOffset + 3 * sizeof(uint32_t) <= mcDataSize, "Effects: data buffer too small (Soap)");
static_assert(sizeof(tOctoMap) * mcCols * mcRows + 2 <= mcDataSize, "Effects: data buffer too small (Octopus)");
static_assert(2 * mcCols * mcRows + 2 * sizeof(uint32_t) <= mcDataSize, "Effects: data buffer too small (Game Of Life)");
static_assert((mcCols + 2) * (mcRows + 2) <= mcDataSize, "Effects: data buffer too small (Sun Radiation)");

/**
 * @brief Returns the description of an effect; 2D effects first, then 1D effects.
 */
static const tEffectInfo* GetEffectInfo(uint8_t aIndex)
{
    if (aIndex < mcEffects2DCount) return &mcEffects2D[aIndex];

    uint8_t wCount1D = 0;
    const tEffectInfo* wEffects1D = GetEffects1D(wCount1D);
    aIndex -= mcEffects2DCount;
    return (aIndex < wCount1D) ? &wEffects1D[aIndex] : nullptr;
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
        default: return PartyColors_p;          // WLED "Default" palette
    }
}

uint8_t GetEffectCount(void)
{
    uint8_t wCount1D = 0;
    GetEffects1D(wCount1D);
    return std::min<int>(mcEffects2DCount + wCount1D, mcMaxEffects);
}

const char* const* GetEffectNames(void)
{
    static char        wBuffer[mcMaxEffects][mcNameLength];
    static const char* wNames[mcMaxEffects] = {};

    if (wNames[0] == nullptr)
    {
        for (uint8_t wI = 0; wI < GetEffectCount(); wI++)
        {
            const tEffectInfo* wInfo = GetEffectInfo(wI);
            snprintf(wBuffer[wI], mcNameLength, "%s (%s)", wInfo->mName, (wInfo->mType == TYPE_2D) ? "2D" : "1D");
            wNames[wI] = wBuffer[wI];
        }
    }
    return wNames;
}

uint8_t GetPaletteCount(void)
{
    return mcPaletteCount;
}

const char* const* GetPaletteNames(void)
{
    return mcPaletteNames;
}

uint8_t GetMappingCount(void)
{
    return mcMappingCount;
}

const char* const* GetMappingNames(void)
{
    return mcMappingNames;
}


/***************************************************************************************************
  Engine
 **************************************************************************************************/
/**
 * @brief Selects an effect and 1D mapping (0 = effect default) and resets the effect state.
 */
void Engine::Start(uint8_t aEffect, uint8_t aMapping)
{
    mEffect         = (aEffect < GetEffectCount()) ? aEffect : 0;
    mMappingSetting = (aMapping < mcMappingCount) ? aMapping : 0;

    const tEffectInfo* wInfo = GetEffectInfo(mEffect);
    mFx.mCustom1 = wInfo->mCustom1;
    mFx.mCustom2 = wInfo->mCustom2;
    mFx.mCustom3 = wInfo->mCustom3;
    mFx.mCheck1  = wInfo->mCheck1;
    mFx.mCheck2  = wInfo->mCheck2;
    mFx.mCheck3  = wInfo->mCheck3;

    mFx.mType    = wInfo->mType;
    mFx.mMapping = (mMappingSetting == 0) ? wInfo->mMapping : static_cast<tMapping>(mMappingSetting - 1);

    switch (mFx.mType)
    {
        case TYPE_1D:
            switch (mFx.mMapping)
            {
                case MAP_BAR:    mFx.mLen = Fx::mRows; break;
                case MAP_CIRCLE: mFx.mLen = int(sqrtf(float(Fx::mCols * Fx::mCols + Fx::mRows * Fx::mRows)) / 2.0f) + 1; break;
                case MAP_CORNER: mFx.mLen = std::max<int>(Fx::mCols, Fx::mRows); break;
                default:         mFx.mLen = Fx::mCols * Fx::mRows; break;
            }
            mFx.mStrips = 1;
            break;

        case TYPE_1D_COLS:
            mFx.mLen    = Fx::mRows;
            mFx.mStrips = Fx::mCols;
            break;

        default:
            mFx.mLen    = Fx::mCols * Fx::mRows;
            mFx.mStrips = 1;
            break;
    }

    mFx.mCall  = 0;
    mFx.mStep  = 0;
    mFx.mAux0  = 0;
    mFx.mAux1  = 0;
    mFx.mDelay = 0;
    mNextFrame = 0;
    memset(mFx.mData, 0, sizeof(mFx.mData));
    fill_solid(mFx.mStrip, Fx::mCols * Fx::mRows, CRGB::Black);
    fill_solid(mFx.mLeds,  Fx::mCols * Fx::mRows, CRGB::Black);

    ApplyPalette();
}

/**
 * @brief Sets the user parameters (0..255) and the palette index.
 */
void Engine::SetParams(uint8_t aSpeed, uint8_t aIntensity, uint8_t aPalette)
{
    mFx.mSpeed     = aSpeed;
    mFx.mIntensity = aIntensity;
    mUserPalette   = (aPalette < mcPaletteCount) ? aPalette : 0;
    ApplyPalette();
}

/**
 * @brief Sets the three effect colors (WLED primary, secondary, tertiary).
 */
void Engine::SetColors(const CRGB& arColor1, const CRGB& arColor2, const CRGB& arColor3)
{
    mFx.mColors[0] = arColor1;
    mFx.mColors[1] = arColor2;
    mFx.mColors[2] = arColor3;
    ApplyPalette();
}

/**
 * @brief Resolves "Default" to the effect's default palette and loads it.
 */
void Engine::ApplyPalette(void)
{
    const tEffectInfo* wInfo = GetEffectInfo(mEffect);
    const uint8_t wDefault = (wInfo != nullptr) ? wInfo->mPalette : 0;

    mFx.mPaletteIndex = (mUserPalette != 0) ? mUserPalette : wDefault;
    mFx.mPalette      = GetPalette(mFx.mPaletteIndex, mFx.mColors);
}

/**
 * @brief Sets the text shown by the scrolling text effect.
 */
void Engine::SetText(const char* apText)
{
    strncpy(mFx.mText, apText, sizeof(mFx.mText) - 1);
    mFx.mText[sizeof(mFx.mText) - 1] = '\0';
}

/**
 * @brief Renders the next frame of the selected effect (unless it requested a longer frame delay).
 */
void Engine::Render(uint32_t aNow)
{
    const tEffectInfo* wInfo = GetEffectInfo(mEffect);
    if ((wInfo == nullptr) || (static_cast<int32_t>(aNow - mNextFrame) < 0)) return;

    mFx.mNow   = aNow;
    mFx.mDelay = 0;
    wInfo->mFn(mFx);
    mFx.mCall++;
    mNextFrame = aNow + mFx.mDelay;

    mFx.Project();
}

}; /* end of namespace EffectsNS */
