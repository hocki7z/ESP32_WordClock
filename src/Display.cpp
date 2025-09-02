/*
 * Display.h
 *
 *  Created on: 02.09.2025
 *      Author: hocki
 */

#include <FastLED.h>

#include "Display.h"


/***************************************************************************************************
  FastLED configuration
 **************************************************************************************************/
/* The LED stripe type                      */
#define LED_TYPE                    WS2812
/* The color order or the LED stripe        */
#define LED_COLOR_ORDER             GRB
/* Digital LED stripe connection pin - D32  */
#define LED_DATA_PIN                32
/* Numberrs of LEDs                         */
#define NUM_LEDS                    1
/* Default LEDs brightness                  */
#define LED_DEFAULT_BRIGHTNESS      255

/* Leds */
static CRGB mLeds[NUM_LEDS];

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
    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, LED_COLOR_ORDER>(mLeds, NUM_LEDS);
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
    mLeds[0] = CRGB::Red;
    FastLED.show();
    delay(1000);

    mLeds[0] = CRGB::Black;
    FastLED.show();
    delay(1000);
}