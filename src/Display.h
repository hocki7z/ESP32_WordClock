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


/***************************************************************************************************
  WordClock configuration
 **************************************************************************************************/

/* WordClock front panel layout (german) */
static const char* mDisplayLayout[MATRIX_HEIGHT]=
{
    /*          0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 */
    /* 00 */  " A  L  A  R  M  G  E  B  U  R  T  S  T  A  G  W  "    // Alarm Geburtstag W
    /* 01 */  " M  Ü  L  L  A  U  T  O  F  E  I  E  R  T  A  G  "    // Müll Auto Feiertag
    /* 02 */  " A  F  O  R  M  E  L  1  D  O  W  N  L  O  A  D  "    // A Formel1 Download
    /* 03 */  " W  L  A  N  U  P  D  A  T  E  R  A  U  S  E  S  "    // Wlan Update Raus Es
    /* 04 */  " B  R  I  N  G  E  N  I  S  T  G  E  L  B  E  R  "    // Bringen Ist Gelber
    /* 05 */  " S  A  C  K  Z  E  I  T  Z  W  A  N  Z  I  G  F  "    // Sack Zeit Zwanzig F
    /* 06 */  " H  A  L  B  G  U  R  L  A  U  B  G  E  N  A  U  "    // Halb G Urlaub Genau
    /* 07 */  " Z  E  H  N  W  E  R  K  S  T  A  T  T  Z  U  M  "    // Zehn Werkstatt Zum
    /* 08 */  " F  Ü  N  F  R  I  S  E  U  R  Z  O  C  K  E  N  "    // Fün Friseur Zocken
    /* 09 */  " W  O  R  D  C  L  O  C  K  V  I  E  R  T  E  L  "    // Wordclock Viertel
    /* 10 */  " V  O  R  N  E  U  S  T  A  R  T  E  R  M  I  N  "    // Vor Neustar Termin
    /* 11 */  " N  A  C  H  L  H  A  L  B  V  S  I  E  B  E  N  "    // Nach L Halb V Sieben
    /* 12 */  " S  E  C  H  S  N  E  U  N  Z  E  H  N  E  L  F  "    // Sechs Neun Zehn Elf
    /* 13 */  " E  I  N  S  D  R  E  I  V  I  E  R  Z  W  E  I  "    // Eins Drei Vier Zwei
    /* 14 */  " A  C  H  T  Z  W  Ö  L  F  Ü  N  F  U  U  H  R  "    // Acht Zwölf ünf U Uhr
    /* 15 */  " S  +  1  2  3  4  O  K  M  I  N  U  T  E  N  W  "    // S + 1 2 3 4 OK Minuten W
};




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

    void Clear(void);
    void Transform(void);

    void SetColor(const uint16_t aLedIndex, const CRGB aColor);

    void PaintPixel(const uint16_t aRow, const uint16_t aCol, const CRGB aColor);
    void PaintLine(const uint16_t aRow, const uint16_t aCol, const uint16_t aLength, const CRGB aColor);
    void PaintArea(const uint16_t aRow, const uint16_t aCol, const uint16_t aWidth, const uint16_t aHeight, const CRGB aColor);
};