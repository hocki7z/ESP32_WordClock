/*
 * DisplayLayout_de_12h.h
 *
 *  Created on: 10.11.2026
 *      Author: hocki
 *
 * Active German 16x16 layout: two colloquial 12-hour modes, notification
 * vocabulary, and a separate bottom-row +1 Minute / +2..4 Minuten display.
 * Included by DisplayLayout.h after its shared tWord and tWordData types.
 * Do not include beside either alternative 24h layout: names are shared.
 *
 * Time composition (civil input hour 0..23, minute 0..59):
 * - Main minute index = minute / 5; remainder index = minute % 5.
 * - Apply HOUR_OFFSET_1 before normalizing the hour to index 0..11.
 * - Hour index zero denotes ZWOELF, for both midnight and noon.
 * - ES IST, the main time, and extra minutes are independent scene groups.
 * - GENAU remains selected for :00..:04 even when an extra minute is shown.
 *
 * Wessi uses ZEHN VOR HALB at :20 and ZEHN NACH HALB at :40.
 * Rhein-Ruhr uses ZWANZIG NACH at :20 and ZWANZIG VOR at :40.
 * For example, 13:21 is ZEHN VOR HALB ZWEI +1 MINUTE in Wessi,
 * or ZWANZIG NACH EINS +1 MINUTE in Rhein-Ruhr (optional ES IST omitted).
 *
 * All coordinates are zero-based horizontal logical spans, independent of
 * physical LED wiring. Overlapping spans are intentional (e.g. FRISEUR and
 * FUENF share an F; ZWOELF and the hour FUENF share an F).
 * Keep table dimensions and C-array types: the renderer copies fixed-width
 * rows with memcpy and sizeof; unused word slots are zero-filled sentinels.
 */
#pragma once

#include "Arduino.h"

/* Matrix and word-table dimensions. Keep C arrays for renderer memcpy/sizeof. */
#define MATRIX_WIDTH                16
#define MATRIX_HEIGHT               16
#define MATRIX_SIZE                 (MATRIX_WIDTH * MATRIX_HEIGHT)
//#define MATRIX_TYPE                 HORIZONTAL_ZIGZAG_MATRIX
#define HOURS_COUNT                 12
#define MINUTE_COUNT                12      // Five-minute steps
#define EXTRA_MINUTE_COUNT          5       // No remainder, then +1..+4
#define MAX_HOUR_WORDS              2
#define MAX_MINUTE_WORDS            3
#define MAX_EXTRA_MINUTE_WORDS      3
#define NO_FLAGS                    0x00
#define HOUR_OFFSET_1               0x01    // Refer to the next hour

/* Printed front-panel cells, including filler letters without a word ID.
 * The preview reads the glyph at column*3+2; preserve padding and case.
 * Each row is 16 cells, encoded as 50 bytes plus the string terminator.
 */
static const char* mDisplayLayout[MATRIX_HEIGHT]=
{
    // To avoid coding problems when logging, the lowercase letters 'u' and 'o'
    // are used instead of the German umlauts 'Ü' and 'Ö'

    /*           0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 */
    /* 00 */  "  A  L  A  R  M  G  E  B  U  R  T  S  T  A  G  W  ",  // Alarm Geburtstag W
    /* 01 */  "  M  u  L  L  A  U  T  O  F  E  I  E  R  T  A  G  ",  // Müll Auto Feiertag
    /* 02 */  "  A  F  O  R  M  E  L  1  D  O  W  N  L  O  A  D  ",  // A Formel1 Download
    /* 03 */  "  W  L  A  N  U  P  D  A  T  E  R  A  U  S  E  S  ",  // Wlan Update Raus Es
    /* 04 */  "  B  R  I  N  G  E  N  I  S  T  G  E  L  B  E  R  ",  // Bringen Ist Gelber
    /* 05 */  "  S  A  C  K  Z  E  I  T  Z  W  A  N  Z  I  G  F  ",  // Sack Zeit Zwanzig F
    /* 06 */  "  H  A  L  B  G  U  R  L  A  U  B  G  E  N  A  U  ",  // Halb G Urlaub Genau
    /* 07 */  "  Z  E  H  N  W  E  R  K  S  T  A  T  T  Z  U  M  ",  // Zehn Werkstatt Zum
    /* 08 */  "  F  u  N  F  R  I  S  E  U  R  Z  O  C  K  E  N  ",  // Fünf Friseur Zocken
    /* 09 */  "  W  O  R  D  C  L  O  C  K  V  I  E  R  T  E  L  ",  // Wordclock Viertel
    /* 10 */  "  V  O  R  N  E  U  S  T  A  R  T  E  R  M  I  N  ",  // Vor Neustart Termin
    /* 11 */  "  N  A  C  H  L  H  A  L  B  V  S  I  E  B  E  N  ",  // Nach L Halb V Sieben
    /* 12 */  "  S  E  C  H  S  N  E  U  N  Z  E  H  N  E  L  F  ",  // Sechs Neun Zehn Elf
    /* 13 */  "  E  I  N  S  D  R  E  I  V  I  E  R  Z  W  E  I  ",  // Eins Drei Vier Zwei
    /* 14 */  "  A  C  H  T  Z  W  o  L  F  u  N  F  U  U  H  R  ",  // Acht Zwölf ünf U Uhr
    /* 15 */  "  S  +  1  2  3  4  O  K  M  I  N  U  T  E  N  W  "   // S + 1 2 3 4 OK Minuten W
};

/* Single source for word IDs and { row, column, length }. Keep existing order
 * (including the public WORD_WEKRSTATT spelling); append new words only.
 * Distinct HALB spans are intentional and must not be merged.
 * Entries: sentinel; minute words; hours 1..12; connectors and extra minutes;
 * notification vocabulary; appended EIN (without S) for full-hour grammar.
 * The zero-length sentinel never illuminates a cell. ID order matches the
 * coordinate array exactly because both are expanded from this same list.
 */
#define WC12_WORDS(X) \
    X(WORD_END_OF_WORDS,  0,  0,  0) \
    X(WORD_CLOCK_MIN_5,   8,  0,  4) \
    X(WORD_CLOCK_MIN_10,  7,  0,  4) \
    X(WORD_CLOCK_MIN_20,  5,  8,  7) \
    X(WORD_CLOCK_MIN_30,  6,  0,  4) \
    X(WORD_CLOCK_HOUR_1, 13,  0,  4) \
    X(WORD_CLOCK_HOUR_2, 13, 12,  4) \
    X(WORD_CLOCK_HOUR_3, 13,  4,  4) \
    X(WORD_CLOCK_HOUR_4, 13,  8,  4) \
    X(WORD_CLOCK_HOUR_5, 14,  8,  4) \
    X(WORD_CLOCK_HOUR_6, 12,  0,  5) \
    X(WORD_CLOCK_HOUR_7, 11, 10,  6) \
    X(WORD_CLOCK_HOUR_8, 14,  0,  4) \
    X(WORD_CLOCK_HOUR_9, 12,  5,  4) \
    X(WORD_CLOCK_HOUR_10,12,  9,  4) \
    X(WORD_CLOCK_HOUR_11,12, 13,  3) \
    X(WORD_CLOCK_HOUR_12,14,  4,  5) \
    X(WORD_ES,            3, 14,  2) \
    X(WORD_IST,           4,  7,  3) \
    X(WORD_GENAU,         6, 11,  5) \
    X(WORD_VIERTEL,       9,  9,  7) \
    X(WORD_HALB,         11,  5,  4) \
    X(WORD_VOR,          10,  0,  3) \
    X(WORD_NACH,         11,  0,  4) \
    X(WORD_UHR,          14, 13,  3) \
    X(WORD_PLUS,         15,  1,  1) \
    X(WORD_NUM_1,        15,  2,  1) \
    X(WORD_NUM_2,        15,  3,  1) \
    X(WORD_NUM_3,        15,  4,  1) \
    X(WORD_NUM_4,        15,  5,  1) \
    X(WORD_MINUTE,       15,  8,  6) \
    X(WORD_MINUTEN,      15,  8,  7) \
    X(WORD_ALARM,         0,  0,  5) \
    X(WORD_GEBURTSTAG,    0,  5, 10) \
    X(WORD_WLAN,          3,  0,  4) \
    X(WORD_MUELL,         1,  0,  4) \
    X(WORD_AUTO,          1,  4,  4) \
    X(WORD_FEIERTAG,      1,  8,  8) \
    X(WORD_FORMEL1,       2,  1,  7) \
    X(WORD_DOWNLOAD,      2,  8,  8) \
    X(WORD_UPDATE,        3,  4,  6) \
    X(WORD_RAUS,          3, 10,  4) \
    X(WORD_BRINGEN,       4,  0,  7) \
    X(WORD_GELBER,        4, 10,  6) \
    X(WORD_SACK,          5,  0,  4) \
    X(WORD_ZEIT,          5,  4,  4) \
    X(WORD_URLAUB,        6,  5,  6) \
    X(WORD_WEKRSTATT,     7,  4,  9) \
    X(WORD_FRISEUR,       8,  3,  7) \
    X(WORD_ZOCKEN,        8, 10,  6) \
    X(WORD_WORDCLOCK,     9,  0,  9) \
    X(WORD_NEUSTART,     10,  3,  8) \
    X(WORD_TERMIN,       10, 10,  6) \
    X(WORD_CLOCK_HOUR_EIN,13, 0,  3)

/** @brief Byte-sized word IDs; WORD_MAX_NUMBER is a count, not a word. */
enum Word : tWord
{
#define WC12_ENUM(aName, aRow, aColumn, aLength) aName,
    WC12_WORDS(WC12_ENUM)
#undef WC12_ENUM
    WORD_MAX_NUMBER
};

/** @brief ID-indexed logical spans: row, starting column, number of LEDs. */
const tWordData mcWordDataArray[WORD_MAX_NUMBER] =
{
#define WC12_DATA(aName, aRow, aColumn, aLength) { aRow, aColumn, aLength },
    WC12_WORDS(WC12_DATA)
#undef WC12_DATA
};
#undef WC12_WORDS

/** @brief Hour phrasing chosen by each five-minute entry. */
enum tHourMode
{
    HOUR_MODE_0, HOUR_MODE_1, HOUR_MODE_MAX_NUMBER // With / without UHR
};
/** @brief Stable mode IDs, also used by the settings selector. */
enum tWordClockMode
{
    WORDCLOCK_MODE_0 = 0, WORDCLOCK_MODE_1, WORDCLOCK_MODE_NUMBER // Wessi / Rhein-Ruhr
};
/** @brief One five-minute phrase and its hour-selection rules.
 * mHourMode selects a row of mcWordHoursTable; mFlags may advance the hour.
 * wMinuteWords contains up to three word IDs, padded with zero when shorter.
 */
struct tMinuteDisplay
{
    tHourMode mHourMode;
    uint8_t   mFlags;
    tWord     wMinuteWords[MAX_MINUTE_WORDS];
};

/* Selector labels follow enum order; Rhein-Ruhr is the layout default. */
static constexpr const char* mcClockModeItems[WORDCLOCK_MODE_NUMBER] = { "Wessi", "Rhein-Ruhr" };
static constexpr tWordClockMode mcDefaultDisplayClockMode = WORDCLOCK_MODE_1;

static_assert(WORD_END_OF_WORDS == 0, "Unused word slots must remain zero");
static_assert(WORD_MAX_NUMBER <= 256, "Word IDs must fit tWord");

/* [hour phrasing][normalized hour 0..11][word slot].
 * The with-UHR row uses EIN; the without-UHR row retains EINS.
 */
const tWord mcWordHoursTable[HOUR_MODE_MAX_NUMBER][HOURS_COUNT][MAX_HOUR_WORDS] =
{
    {
        { WORD_CLOCK_HOUR_12,   WORD_UHR },
        { WORD_CLOCK_HOUR_EIN,  WORD_UHR }, // EIN UHR; keep EINS for the without-UHR row
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

/* [clock mode][minute/5][phrase]. Entries correspond to :00,:05,...,:55.
 * Flags specify the referenced hour, not rounding: remainders are additive.
 */
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

/* [minute%5][word slot]. Row zero is empty; only +1 uses singular MINUTE.
 * These LEDs are separate from the main phrase and may use their own color.
 */
const tWord mcWordExtraMinutesTable[EXTRA_MINUTE_COUNT][MAX_EXTRA_MINUTE_WORDS] =
{
    { WORD_END_OF_WORDS },
    { WORD_PLUS, WORD_NUM_1, WORD_MINUTE  },
    { WORD_PLUS, WORD_NUM_2, WORD_MINUTEN },
    { WORD_PLUS, WORD_NUM_3, WORD_MINUTEN },
    { WORD_PLUS, WORD_NUM_4, WORD_MINUTEN }
};
