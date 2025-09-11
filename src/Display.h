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
/*  12-hour display time  */
#define HOURS_COUNT                 12
/* Number of minute steps (5min) */
#define MINUTE_COUNT                12
/* Number of extra minutes ( +1 ... +4 ) */
#define EXTRA_MINUTE_COUNT          5

/* Maximum number of words to display the hours */
#define MAX_HOUR_WORDS              2
/* Maximum number of words to display the minutes */
#define MAX_MINUTE_WORDS            3
/* Maximum number of words to display the minutes */
#define MAX_EXTRA_MINUTE_WORDS      3

/* Flags for minute display */
#define NO_FLAGS                    0x00    // No flags
#define HOUR_OFFSET_1               0x01    // Hour offset +1 (e.g. for minutes > 20)


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

/* List of all words */
typedef enum tWord
{
    WORD_END_OF_WORDS,

    /* WordClock: minutes words */
    WORD_CLOCK_MIN_5,   // Fünf
    WORD_CLOCK_MIN_10,  // Zehn
    WORD_CLOCK_MIN_20,  // Zwanzig
    WORD_CLOCK_MIN_30,  // Halb
    /* WordClock: hours words */
    WORD_CLOCK_HOUR_1,  // Eins
    WORD_CLOCK_HOUR_2,  // Zwei
    WORD_CLOCK_HOUR_3,  // Drei
    WORD_CLOCK_HOUR_4,  // Vier
    WORD_CLOCK_HOUR_5,  // Fünf
    WORD_CLOCK_HOUR_6,  // Sechs
    WORD_CLOCK_HOUR_7,  // Sieben
    WORD_CLOCK_HOUR_8,  // Acht
    WORD_CLOCK_HOUR_9,  // Neun
    WORD_CLOCK_HOUR_10, // Zehn
    WORD_CLOCK_HOUR_11, // Elf
    WORD_CLOCK_HOUR_12, // Zwölf
    /* WordClock: special words */
    WORD_ES,
    WORD_IST,
    WORD_GENAU,
    WORD_VIERTEL,
    WORD_HALB,
    WORD_VOR,
    WORD_NACH,
    WORD_UHR,
    WORD_PLUS,
    WORD_NUM_1,
    WORD_NUM_2,
    WORD_NUM_3,
    WORD_NUM_4,
    WORD_MINUTE,
    WORD_MINUTEN,
    /* Additional words */
    WORD_ALARM,
    WORD_GEBURTSTAG,
    WORD_WLAN,
    WORD_MUELL,
    WORD_AUTO,
    WORD_FEIERTAG,
    WORD_FORMEL1,
    WORD_DOWNLOAD,
    WORD_UPDATE,
    WORD_RAUS,
    WORD_BRINGEN,
    WORD_GELBER,
    WORD_SACK,
    WORD_ZEIT,
    WORD_URLAUB,
    WORD_WEKRSTATT,
    WORD_FRISEUR,
    WORD_ZOCKEN,
    WORD_WORDCLOCK,
    WORD_NEUSTART,
    WORD_TERMIN,
    //
    WORD_MAX_NUMBER
} tWord;

/* Hour display modes */
typedef enum tHourMode
{
    HOUR_MODE_0,          // Hours with "Uhr" (standard)
    HOUR_MODE_1,          // Hours without "Uhr"
    //
    HOUR_MODE_MAX_NUMBER
} tHourMode;

/* WordClock display modes */
typedef enum tWordClockMode
{
    WORDCLOCK_MODE_0 = 0,   // Wessi
    WORDCLOCK_MODE_1,       // Rhein-Ruhr
    //
    WORDCLOCK_MODE_NUMBER
} tWordClockMode;



class Display
{
public:
    Display();
    virtual ~Display();

    void Init(void);

    void Update(void);


private:
    /* Struct to store word data */
    typedef struct tWordData
    {
        /* Row index [0..MATRIX_HEIGHT-1] */
        uint8_t mRow;
        /* Column index [0..MATRIX_WIDTH-1] */
        uint8_t mColumn;
        /* Word length */
        uint8_t mLength;
    } tWordData;

    /* Stuct to store the data to display minutes */
    typedef struct tMinuteDisplay
    {
        tHourMode mHourMode;
        uint8_t   mFlags;
        tWord     wMinuteWords[MAX_MINUTE_WORDS];
    } tMinuteDisplay;

    /* Word data array */
    const tWordData mcWordDataArray[WORD_MAX_NUMBER] =
    {
        /* WORD_END_OF_WORDS  */   {  0,  0,  0 },    // !!! End of words marker !!!

        /* WORD_CLOCK_MIN_5   */   {  8,  0,  4 },    // Fünf
        /* WORD_CLOCK_MIN_10  */   {  7,  0,  4 },    // Zehn
        /* WORD_CLOCK_MIN_20  */   {  5,  8,  7 },    // Zwanzig
        /* WORD_CLOCK_MIN_30  */   {  6,  0,  4 },    // Halb
        /* WORD_CLOCK_HOUR_1  */   { 13,  0,  4 },    // Eins
        /* WORD_CLOCK_HOUR_2  */   { 13, 12,  4 },    // Zwei
        /* WORD_CLOCK_HOUR_3  */   { 13,  4,  4 },    // Drei
        /* WORD_CLOCK_HOUR_4  */   { 13,  8,  4 },    // Vier
        /* WORD_CLOCK_HOUR_5  */   { 14,  8,  4 },    // Fünf
        /* WORD_CLOCK_HOUR_6  */   { 12,  0,  5 },    // Sechs
        /* WORD_CLOCK_HOUR_7  */   { 11, 10,  6 },    // Sieben
        /* WORD_CLOCK_HOUR_8  */   { 14,  0,  4 },    // Acht
        /* WORD_CLOCK_HOUR_9  */   { 12,  5,  4 },    // Neun
        /* WORD_CLOCK_HOUR_10 */   { 12,  9,  4 },    // Zehn
        /* WORD_CLOCK_HOUR_11 */   { 12, 13,  3 },    // Elf
        /* WORD_CLOCK_HOUR_12 */   { 14,  4,  5 },    // Zwölf
        /* WORD_ES            */   {  3, 14,  2 },    // Es
        /* WORD_IST           */   {  4,  7,  3 },    // Ist
        /* WORD_GENAU         */   {  6, 11,  5 },    // Genau
        /* WORD_VIERTEL       */   {  9,  9,  7 },    // Viertel
        /* WORD_HALB          */   { 11,  5,  4 },    // Halb
        /* WORD_VOR           */   { 10,  0,  3 },    // Vor
        /* WORD_NACH          */   { 11,  0,  4 },    // Nach
        /* WORD_UHR           */   { 14, 13,  3 },    // Uhr
        /* WORD_PLUS          */   { 15,  1,  1 },    // +
        /* WORD_NUM_1         */   { 15,  2,  1 },    // 1
        /* WORD_NUM_2         */   { 15,  3,  1 },    // 2
        /* WORD_NUM_3         */   { 15,  4,  1 },    // 3
        /* WORD_NUM_4         */   { 15,  5,  1 },    // 4
        /* WORD_MINUTE        */   { 15,  8,  6 },    // Minute
        /* WORD_MINUTEN       */   { 15,  8,  7 },    // Minuten
        /* WORD_ALARM         */   {  0,  0,  5 },    // Alarm
        /* WORD_GEBURTSTAG    */   {  0,  5, 10 },    // Geburtstag
        /* WORD_WLAN          */   {  3,  0,  4 },    // Wlan
        /* WORD_MUELL         */   {  1,  0,  4 },    // Müll
        /* WORD_AUTO          */   {  1,  4,  4 },    // Auto
        /* WORD_FEIERTAG      */   {  1,  8,  8 },    // Feiertag
        /* WORD_FORMEL1       */   {  2,  1,  7 },    // Formel1
        /* WORD_DOWNLOAD      */   {  2,  8,  8 },    // Download
        /* WORD_UPDATE        */   {  3,  4,  6 },    // Update
        /* WORD_RAUS          */   {  3, 10,  4 },    // Raus
        /* WORD_BRINGEN       */   {  4,  0,  7 },    // Bringen
        /* WORD_GELBER        */   {  4, 10,  6 },    // Gelber
        /* WORD_SACK          */   {  5,  0,  4 },    // Sack
        /* WORD_ZEIT          */   {  5,  4,  4 },    // Zeit
        /* WORD_URLAUB        */   {  6,  5,  6 },    // Urlaub
        /* WORD_WEKRSTATT     */   {  7,  4,  9 },    // Werkstatt
        /* WORD_FRISEUR       */   {  8,  3,  7 },    // Friesur
        /* WORD_ZOCKEN        */   {  8, 10,  6 },    // Zocken
        /* WORD_WORDCLOCK     */   {  9,  0,  9 },    // Wordclock
        /* WORD_NEUSTART      */   { 10,  3,  8 },    // Neustart
        /* WORD_TERMIN        */   { 10, 10,  6 },    // Termin
    };

    /* Hour words table */
    const tWord mcWordHoursTable[HOUR_MODE_MAX_NUMBER][HOURS_COUNT][MAX_HOUR_WORDS] =
    {
        {
            { WORD_CLOCK_HOUR_12,   WORD_UHR },
            { WORD_CLOCK_HOUR_1,    WORD_UHR },
            { WORD_CLOCK_HOUR_2,    WORD_UHR },
            { WORD_CLOCK_HOUR_3,    WORD_UHR },
            { WORD_CLOCK_HOUR_4,    WORD_UHR },
            { WORD_CLOCK_HOUR_5,    WORD_UHR },
            { WORD_CLOCK_HOUR_6,    WORD_UHR },
            { WORD_CLOCK_HOUR_7,    WORD_UHR },
            { WORD_CLOCK_HOUR_8,    WORD_UHR },
            { WORD_CLOCK_HOUR_9,    WORD_UHR },
            { WORD_CLOCK_HOUR_10,   WORD_UHR },
            { WORD_CLOCK_HOUR_11,   WORD_UHR }
        },
        {
            { WORD_CLOCK_HOUR_12 },
            { WORD_CLOCK_HOUR_1  },
            { WORD_CLOCK_HOUR_2  },
            { WORD_CLOCK_HOUR_3  },
            { WORD_CLOCK_HOUR_4  },
            { WORD_CLOCK_HOUR_5  },
            { WORD_CLOCK_HOUR_6  },
            { WORD_CLOCK_HOUR_7  },
            { WORD_CLOCK_HOUR_8  },
            { WORD_CLOCK_HOUR_9  },
            { WORD_CLOCK_HOUR_10 },
            { WORD_CLOCK_HOUR_11 }
        },
    };

    /* Minute words table */
    const tMinuteDisplay mcWordMinutesTable[WORDCLOCK_MODE_NUMBER][MINUTE_COUNT] =
    {
        /* Mode WESSI */
        {
            { HOUR_MODE_0, NO_FLAGS,      { WORD_GENAU                                }},       // 00
            { HOUR_MODE_1, NO_FLAGS,      { WORD_CLOCK_MIN_5,   WORD_NACH             }},       // 05
            { HOUR_MODE_1, NO_FLAGS,      { WORD_CLOCK_MIN_10,  WORD_NACH             }},       // 10
            { HOUR_MODE_1, NO_FLAGS,      { WORD_VIERTEL,       WORD_NACH             }},       // 15
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_10,  WORD_VOR,   WORD_HALB }},       // 20
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_5,   WORD_VOR,   WORD_HALB }},       // 25
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_HALB                                 }},       // 30
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_5,   WORD_NACH,  WORD_HALB }},       // 35
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_10,  WORD_NACH,  WORD_HALB }},       // 40
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_VIERTEL,       WORD_VOR              }},       // 45
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_10,  WORD_VOR              }},       // 50
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_5,   WORD_VOR              }},       // 55
        },

        /* Mode RHEIN-RUHR */
        {
            { HOUR_MODE_0, NO_FLAGS,      { WORD_GENAU                                }},       // 00
            { HOUR_MODE_1, NO_FLAGS,      { WORD_CLOCK_MIN_5,   WORD_NACH             }},       // 05
            { HOUR_MODE_1, NO_FLAGS,      { WORD_CLOCK_MIN_10,  WORD_NACH             }},       // 10
            { HOUR_MODE_1, NO_FLAGS,      { WORD_VIERTEL,       WORD_NACH             }},       // 15
            { HOUR_MODE_1, NO_FLAGS,      { WORD_CLOCK_MIN_20,  WORD_NACH             }},       // 20
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_5,   WORD_VOR,   WORD_HALB }},       // 25
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_HALB                                 }},       // 30
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_5,   WORD_NACH,  WORD_HALB }},       // 35
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_20,  WORD_VOR              }},       // 40
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_VIERTEL,       WORD_VOR              }},       // 45
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_10,  WORD_VOR              }},       // 50
            { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_5,   WORD_VOR              }},       // 55
        }
    };

    const tWord mcWordExtraMinutesTable[ EXTRA_MINUTE_COUNT ][ MAX_EXTRA_MINUTE_WORDS ] =
    {
        { WORD_END_OF_WORDS                       },    // No extra minutes
        { WORD_PLUS,   WORD_NUM_1,  WORD_MINUTE   },    // +1 Minute
        { WORD_PLUS,   WORD_NUM_2,  WORD_MINUTEN  },    // +2 Minutes
        { WORD_PLUS,   WORD_NUM_3,  WORD_MINUTEN  },    // +3 Minutes
        { WORD_PLUS,   WORD_NUM_4,  WORD_MINUTEN  },    // +4 Minutes
    };

    /* Leds */
    CRGB mLeds[LED_NUMBER];

    uint32_t mPrevMillis = 0;

    void Clear(void);
    void Transform(void);

    void SetColor(const uint16_t aLedIndex, const CRGB aColor);

    void PaintPixel(const uint16_t aRow, const uint16_t aCol, const CRGB aColor);
    void PaintLine(const uint16_t aRow, const uint16_t aCol, const uint16_t aLength, const CRGB aColor);
    void PaintArea(const uint16_t aRow, const uint16_t aCol, const uint16_t aWidth, const uint16_t aHeight, const CRGB aColor);

    void PaintWord(const tWord aWord, const CRGB aColor);
    void PaintTime(const uint8_t aHour, const uint8_t aMinute, const CRGB aColor);
};