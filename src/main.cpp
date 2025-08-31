#include <Arduino.h>

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

/* Leds */
CRGB mLeds[NUM_LEDS];


void setup()
{
    /* Start the serial communication */
    Serial.begin(115200);
    Serial.setDebugOutput(true);

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

    /* LOG */
    Serial.printf("Welcome to WordClock");
}

void loop()
{
    mLeds[0] = CRGB::Red;
    FastLED.show();
    delay(1000);

    mLeds[0] = CRGB::Black;
    FastLED.show();
    delay(1000);
}
