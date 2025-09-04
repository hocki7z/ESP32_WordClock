/*
 * Display.h
 *
 *  Created on: 02.09.2025
 *      Author: hocki
 */

#include <FastLED.h>


/***************************************************************************************************
  LED matrix configuration
 **************************************************************************************************/
#define MATRIX_WIDTH                16
#define MATRIX_HEIGHT               16
#define MATRIX_SIZE                 MATRIX_WIDTH * MATRIX_HEIGHT
//#define MATRIX_TYPE                 HORIZONTAL_ZIGZAG_MATRIX

/***************************************************************************************************
  FastLED configuration
 **************************************************************************************************/
/* The LED stripe type                      */
#define LED_TYPE                    WS2812
/* The color order or the LED stripe        */
#define LED_COLOR_ORDER             GRB
/* Digital LED stripe connection pin - D32  */
#define LED_DATA_PIN                32
/* Default LEDs brightness                  */
#define LED_DEFAULT_BRIGHTNESS      255
/* Numbers of LEDs                          */
#define LED_NUMBER                  MATRIX_SIZE




class Display
{
public:
    Display();
    virtual ~Display();

    void Init(void);

    void Update(void);


private:
    /* Leds */
    CRGB mLeds[LED_NUMBER];

    uint32_t mPrevMillis = 0;

    uint8_t mColorIndex = 0;
};