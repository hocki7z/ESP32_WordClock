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

/* Lenght of fixed colors array */
constexpr uint8_t mcColorLen = 4;
/* Colors array */
constexpr CRGB    mcColors[mcColorLen] = { CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Black };


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

        /* Change LED color and update it */
        mLeds[0] = mcColors[mColorIndex];
        FastLED.show();

        /* Incement color index and check overflow */
        mColorIndex++;
        if (mColorIndex == mcColorLen)
        {
            mColorIndex = 0;
        }
    }
}