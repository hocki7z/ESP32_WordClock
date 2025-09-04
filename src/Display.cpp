/*
 * Display.h
 *
 *  Created on: 02.09.2025
 *      Author: hocki
 */

#include <Arduino.h>

#include "Display.h"

/* Delay in msec between colors */
constexpr uint32_t mcUpdateDelay = 1000;


/**
 * @brief Constructor
 */
Display::Display()
{
    // do nothing
}

/**
 * @brief Destructor
 */
Display::~Display()
{
    // do nothing
}

void Display::Init(void)
{
    /* Initialize FastLED */
    // Initialize LEDs
    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, LED_COLOR_ORDER>(mLeds, LED_NUMBER);
    // Disable dithering mode
	FastLED.setDither(DISABLE_DITHER);
    // Color-corrected LED brightness
    FastLED.setCorrection(TypicalLEDStrip);
    // Set default LED brightness
    FastLED.setBrightness(LED_DEFAULT_BRIGHTNESS);
    // Switch OFF all LEDs
    FastLED.clear();
}

void Display::Update(void)
{
    /* Get current system tick */
    uint32_t wCurrMillis = millis();

    //TODO handle millis rollover
    //     e.g. https://arduino.stackexchange.com/questions/12587/how-can-i-handle-the-millis-rollover

    /* Check time ticks delta */
    if ((wCurrMillis - mPrevMillis) >= mcUpdateDelay)
    {
        /* Update previous time tick */
        mPrevMillis = wCurrMillis;

        Clear();

        Transform();

        FastLED.show();
    }
}

void Display::Clear(void)
{
    /* Clear mLeds array with black color */
    for (uint16_t wI = 0; wI < LED_NUMBER; wI++)
    {
        mLeds[wI] = CRGB::Black;
    }
}

void Display::Transform(void)
{
    for (uint8_t wRow = 0; wRow < MATRIX_HEIGHT; wRow++)
    {
        bool wReverseRow = false;

        if ((wRow % 2) == 0)
        {
            // it is even row
            wReverseRow = true;
        }
        else
        {
            // it is odd row
        }

        if (wReverseRow)
        {
            uint16_t wRowStart = (wRow * MATRIX_WIDTH); //  e.g. 0 / 16 / 32 / 48 ... 224 / 240

            /* Pointer to first element */
            CRGB* wLeftElem  = &mLeds[wRowStart];
            /* Pointer to last element */
            CRGB* wRightElem = &mLeds[wRowStart + MATRIX_WIDTH - 1] ;

            while (wLeftElem < wRightElem)
            {
                CRGB wTemp  = *wLeftElem;
                *wLeftElem  = *wRightElem;
                *wRightElem = wTemp;

                wLeftElem++;
                wRightElem--;
            }
        }
    }
}

void Display::SetColor(const uint16_t aLedIndex, const CRGB aColor)
{
    if (aLedIndex < LED_NUMBER)
    {
        mLeds[aLedIndex] = aColor;
    }
}

void Display::PaintPixel(const uint16_t aRow, const uint16_t aCol, const CRGB aColor)
{
    /* Check input parameters */
    if ((aRow < MATRIX_HEIGHT) &&
        (aCol < MATRIX_WIDTH))
    {
        /* Calculate LED index */
        uint16_t wLedIndex = (aRow * MATRIX_HEIGHT) + aCol;

        /* Set LED color */
        SetColor(wLedIndex, aColor);
    }
}

void Display::PaintLine(const uint16_t aRow, const uint16_t aCol, const uint16_t aLength, const CRGB aColor)
{
    /* Check input parameters */
    if ((aRow < MATRIX_HEIGHT) &&
        (aCol < MATRIX_WIDTH)  && ((aCol + aLength) <= MATRIX_WIDTH))
    {
        for (uint16_t wI = 0; wI < aLength; wI++)
        {
            PaintPixel(aRow, (aCol + wI), aColor);
        }
    }
}

void Display::PaintArea(const uint16_t aRow, const uint16_t aCol, const uint16_t aWidth, const uint16_t aHeight, const CRGB aColor)
{
    /* Check input parameters */
    if ((aRow < MATRIX_HEIGHT) && ((aRow + aHeight) <= MATRIX_HEIGHT) &&
        (aCol < MATRIX_WIDTH)  && ((aCol + aWidth)  <= MATRIX_WIDTH))
    {
        for (uint16_t wI = 0; wI < aHeight; wI++)
        {
            PaintLine((aRow + wI), aCol, aWidth, aColor);
        }
    }
}
