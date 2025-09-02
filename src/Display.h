/*
 * Display.h
 *
 *  Created on: 02.09.2025
 *      Author: hocki
 */

#include <FastLED.h>


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




class Display
{
public:
    Display();
    virtual ~Display();

    void Init(void);

    void Update(void);


private:
    /* Leds */
    CRGB mLeds[NUM_LEDS];

    uint32_t mPrevMillis = 0;

    uint8_t mColorIndex = 0;
};