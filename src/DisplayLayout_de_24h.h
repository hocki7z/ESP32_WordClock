/*
 * DisplayLayout_de_24h.h -- German 16x16 front-panel proposal, 10.11.2026
 *
 * Design references: wordclock24h/make-tables/tables24h-de2.{c,h} (separate
 * hour/minute blocks) and tables24h-de.{c,h} (regional and numeric modes).
 * Reference implementations: Copyright (c) 2018-2019 Frank Meyer,
 * GPL-2.0-or-later. This is a new arrangement and table implementation.
 *
 * PREPARATION ONLY: intentionally NOT included by DisplayLayout.h.
 * Self-contained; the coordinate type mirrors the current tWordData fields.
 * Select INSTEAD OF the 12h header; the shared names cannot coexist.
 *
 * Modes 0/1 retain the existing Wessi/Rhein-Ruhr IDs and wording. Additional
 * modes provide numeric 24h and 12h (exact or five-minute base + remainder),
 * and two regional quarter-hour variants. All stepped modes retain the
 * existing +1 Minute / +2..4 Minuten display and floor-to-five behavior.
 * Exact modes include every minute in words and must NOT add a remainder.
 * The reference's day-period, temperature, countdown and Jester modes are
 * not provided: their extra vocabulary/order does not fit this proposal.
 * Old ancillary notification words are omitted; WORD_WORDCLOCK is retained.
 *
 * Future integration requirements (NO application changes made here):
 * - Use GetLayoutMinuteDisplay(mode, minute) for exact vs. stepped lookup.
 * - Use GetLayoutHourIndex(hour, minuteDisplay) for modulo-24 normalization;
 *   the existing renderer explicitly maps noon to zero and cannot be reused.
 * - Use GetLayoutExtraMinuteIndex(mode, minute) to avoid double-counting.
 * - Increase WordGroup storage from 5 to LAYOUT_MAX_TIME_WORDS (9): the
 *   renderer copies 5 minute slots + 4 hour slots even when some are empty.
 *   Change that capacity BEFORE WordGroup is declared in DisplayLayout.h.
 * - Adapt explicit tWordData references/copies in Display.cpp and WebSite.cpp
 *   to tLayoutWordData24h (or consolidate the shared coordinate type first).
 *   Matching fields do not make these two C++ types interchangeable.
 * - Wire the selected mode into the renderer (currently hardcoded), and
 *   update UI/settings limits while retaining mode IDs 0/1.
 * - Translate u/o/s to the appropriate German glyphs in the web preview;
 *   its current direct character emission displays the placeholders literally.
 *
 * Reading order: colloquial minutes -> hours -> numeric minutes -> extras.
 * Logical cells use the current three-byte preview encoding; 'u' = U-umlaut,
 * 'o' = O-umlaut and 's' = sharp-s. Print single German glyphs, not UE/OE/SS.
 * Coordinates are logical left-to-right spans, not physical LED indices.
 */
#pragma once

#include <array>
#include <initializer_list>
#include <stdint.h>

using tWord = uint8_t;

/* Separate name avoids redefining the owner's existing tWordData. */
struct tLayoutWordData24h
{
    uint8_t mRow;
    uint8_t mColumn;
    uint8_t mLength;
};

#define MATRIX_WIDTH                16
#define MATRIX_HEIGHT               16
#define MATRIX_SIZE                 (MATRIX_WIDTH * MATRIX_HEIGHT)
#define HOURS_COUNT                 24
#define MINUTE_COUNT                12  // Five-minute base table
#define EXACT_MINUTE_COUNT          60
#define EXTRA_MINUTE_COUNT          5
#define MAX_HOUR_WORDS              4   // EIN + UND + ZWANZIG + UHR
#define MAX_MINUTE_WORDS            5   // UND + FUENF + UND + FUENFZIG + MINUTEN
#define MAX_EXTRA_MINUTE_WORDS      3
#define LAYOUT_MAX_TIME_WORDS       (MAX_MINUTE_WORDS + MAX_HOUR_WORDS)
#define NO_FLAGS                    0x00
#define HOUR_OFFSET_1               0x01

/* Compact transcription (16 cells per row):
 *   ESISTZWANZIGFuNF
 *   DREIVIERTELGENAU
 *   ZEHNVORNACHHALBX
 *   NULLAEINSBZWEICD
 *   DREIAVIERBFuNFGH
 *   SECHSASIEBENACHT
 *   NEUNAZEHNBELFUND
 *   ZWoLFZWANZIGUHRX
 *   UNDEINEZWEIDREIA
 *   VIERAFuNFBSECHSC
 *   SIEBENACHTNEUNAB
 *   ZEHNELFBZWoLFUND
 *   ZWANZIGWORDCLOCK
 *   DREIsIGAVIERZIGB
 *   FuNFZIGAMINUTENB
 *   S+1234OKMINUTENW
 */
static constexpr const char* mDisplayLayout[MATRIX_HEIGHT] =
{
    /*           0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 */
    /* 00 */  "  E  S  I  S  T  Z  W  A  N  Z  I  G  F  u  N  F  ",
    /* 01 */  "  D  R  E  I  V  I  E  R  T  E  L  G  E  N  A  U  ",
    /* 02 */  "  Z  E  H  N  V  O  R  N  A  C  H  H  A  L  B  X  ",
    /* 03 */  "  N  U  L  L  A  E  I  N  S  B  Z  W  E  I  C  D  ",
    /* 04 */  "  D  R  E  I  A  V  I  E  R  B  F  u  N  F  G  H  ",
    /* 05 */  "  S  E  C  H  S  A  S  I  E  B  E  N  A  C  H  T  ",
    /* 06 */  "  N  E  U  N  A  Z  E  H  N  B  E  L  F  U  N  D  ",
    /* 07 */  "  Z  W  o  L  F  Z  W  A  N  Z  I  G  U  H  R  X  ",
    /* 08 */  "  U  N  D  E  I  N  E  Z  W  E  I  D  R  E  I  A  ",
    /* 09 */  "  V  I  E  R  A  F  u  N  F  B  S  E  C  H  S  C  ",
    /* 10 */  "  S  I  E  B  E  N  A  C  H  T  N  E  U  N  A  B  ",
    /* 11 */  "  Z  E  H  N  E  L  F  B  Z  W  o  L  F  U  N  D  ",
    /* 12 */  "  Z  W  A  N  Z  I  G  W  O  R  D  C  L  O  C  K  ",
    /* 13 */  "  D  R  E  I  s  I  G  A  V  I  E  R  Z  I  G  B  ",
    /* 14 */  "  F  u  N  F  Z  I  G  A  M  I  N  U  T  E  N  B  ",
    /* 15 */  "  S  +  1  2  3  4  O  K  M  I  N  U  T  E  N  W  "
};

typedef enum Word : tWord
{
    WORD_END_OF_WORDS = 0,
    WORD_CLOCK_MIN_5, WORD_CLOCK_MIN_10, WORD_CLOCK_MIN_20, WORD_CLOCK_MIN_30,
    WORD_CLOCK_HOUR_1, WORD_CLOCK_HOUR_2, WORD_CLOCK_HOUR_3, WORD_CLOCK_HOUR_4,
    WORD_CLOCK_HOUR_5, WORD_CLOCK_HOUR_6, WORD_CLOCK_HOUR_7, WORD_CLOCK_HOUR_8,
    WORD_CLOCK_HOUR_9, WORD_CLOCK_HOUR_10, WORD_CLOCK_HOUR_11, WORD_CLOCK_HOUR_12,
    WORD_ES, WORD_IST, WORD_GENAU, WORD_VIERTEL, WORD_HALB, WORD_VOR, WORD_NACH,
    WORD_UHR, WORD_PLUS, WORD_NUM_1, WORD_NUM_2, WORD_NUM_3, WORD_NUM_4,
    WORD_MINUTE, WORD_MINUTEN, WORD_DREIVIERTEL,
    WORD_CLOCK_HOUR_0, WORD_CLOCK_HOUR_EIN, WORD_CLOCK_HOUR_SECH,
    WORD_CLOCK_HOUR_SIEB, WORD_CLOCK_HOUR_UND, WORD_CLOCK_HOUR_20,
    WORD_CLOCK_MIN_UND_UHR, WORD_CLOCK_MIN_EIN, WORD_CLOCK_MIN_EINE,
    WORD_CLOCK_MIN_2_24H, WORD_CLOCK_MIN_3_24H, WORD_CLOCK_MIN_4_24H,
    WORD_CLOCK_MIN_5_24H, WORD_CLOCK_MIN_6_24H, WORD_CLOCK_MIN_7_24H,
    WORD_CLOCK_MIN_8_24H, WORD_CLOCK_MIN_9_24H, WORD_CLOCK_MIN_10_24H,
    WORD_CLOCK_MIN_11_24H, WORD_CLOCK_MIN_12_24H,
    WORD_CLOCK_MIN_SECH, WORD_CLOCK_MIN_SIEB, WORD_CLOCK_MIN_UND,
    WORD_CLOCK_MIN_20_24H, WORD_CLOCK_MIN_30_24H, WORD_CLOCK_MIN_40_24H,
    WORD_CLOCK_MIN_50_24H, WORD_CLOCK_MIN_MINUTE, WORD_CLOCK_MIN_MINUTEN,
    WORD_WORDCLOCK,
    WORD_MAX_NUMBER
} Word;

typedef enum tHourMode
{
    HOUR_MODE_0,             // 12h with UHR: EIN UHR
    HOUR_MODE_1,             // 12h without UHR: EINS
    HOUR_MODE_24H,           // 00..23 with UHR
    HOUR_MODE_MAX_NUMBER
} tHourMode;

typedef enum tWordClockMode
{
    WORDCLOCK_MODE_0 = 0,    // Existing Wessi ID and :20/:40 behavior
    WORDCLOCK_MODE_1,        // Existing Rhein-Ruhr ID and :20/:40 behavior
    WORDCLOCK_MODE_24H,
    WORDCLOCK_MODE_24H_EXACT,
    // Both regional additions inherit Rhein-Ruhr :20/:40 wording; they are
    // explicit quarter-hour variants, not copies of all DE reference modes.
    WORDCLOCK_MODE_VIERTEL,  // :15 VIERTEL next hour, :45 DREIVIERTEL next hour
    WORDCLOCK_MODE_DREIVIERTEL, // :15 VIERTEL NACH, :45 DREIVIERTEL next hour
    WORDCLOCK_MODE_12H,
    WORDCLOCK_MODE_12H_EXACT,
    WORDCLOCK_MODE_NUMBER
} tWordClockMode;

typedef struct tMinuteDisplay
{
    tHourMode mHourMode;
    uint8_t   mFlags;
    tWord     wMinuteWords[MAX_MINUTE_WORDS];
} tMinuteDisplay;

static constexpr const char* mcClockModeItems[WORDCLOCK_MODE_NUMBER] =
{
    "Wessi", "Rhein-Ruhr", "24h (5 min + extra)", "24h (exact)",
    "Viertel / Dreiviertel", "Viertel nach / Dreiviertel",
    "12h numeric (5 min + extra)", "12h numeric (exact)"
};
static constexpr tWordClockMode mcDefaultDisplayClockMode = WORDCLOCK_MODE_24H_EXACT;

static constexpr tLayoutWordData24h mcWordDataArray[WORD_MAX_NUMBER] =
{
    /* WORD_END_OF_WORDS       */ {  0,  0,  0 },
    /* WORD_CLOCK_MIN_5        */ {  0, 12,  4 },
    /* WORD_CLOCK_MIN_10       */ {  2,  0,  4 },
    /* WORD_CLOCK_MIN_20       */ {  0,  5,  7 },
    /* WORD_CLOCK_MIN_30       */ {  2, 11,  4 },
    /* WORD_CLOCK_HOUR_1       */ {  3,  5,  4 },
    /* WORD_CLOCK_HOUR_2       */ {  3, 10,  4 },
    /* WORD_CLOCK_HOUR_3       */ {  4,  0,  4 },
    /* WORD_CLOCK_HOUR_4       */ {  4,  5,  4 },
    /* WORD_CLOCK_HOUR_5       */ {  4, 10,  4 },
    /* WORD_CLOCK_HOUR_6       */ {  5,  0,  5 },
    /* WORD_CLOCK_HOUR_7       */ {  5,  6,  6 },
    /* WORD_CLOCK_HOUR_8       */ {  5, 12,  4 },
    /* WORD_CLOCK_HOUR_9       */ {  6,  0,  4 },
    /* WORD_CLOCK_HOUR_10      */ {  6,  5,  4 },
    /* WORD_CLOCK_HOUR_11      */ {  6, 10,  3 },
    /* WORD_CLOCK_HOUR_12      */ {  7,  0,  5 },
    /* WORD_ES                 */ {  0,  0,  2 },
    /* WORD_IST                */ {  0,  2,  3 },
    /* WORD_GENAU              */ {  1, 11,  5 },
    /* WORD_VIERTEL            */ {  1,  4,  7 },
    /* WORD_HALB               */ {  2, 11,  4 },
    /* WORD_VOR                */ {  2,  4,  3 },
    /* WORD_NACH               */ {  2,  7,  4 },
    /* WORD_UHR                */ {  7, 12,  3 },
    /* WORD_PLUS               */ { 15,  1,  1 },
    /* WORD_NUM_1              */ { 15,  2,  1 },
    /* WORD_NUM_2              */ { 15,  3,  1 },
    /* WORD_NUM_3              */ { 15,  4,  1 },
    /* WORD_NUM_4              */ { 15,  5,  1 },
    /* WORD_MINUTE             */ { 15,  8,  6 },
    /* WORD_MINUTEN            */ { 15,  8,  7 },
    /* WORD_DREIVIERTEL        */ {  1,  0, 11 },
    /* WORD_CLOCK_HOUR_0       */ {  3,  0,  4 },
    /* WORD_CLOCK_HOUR_EIN     */ {  3,  5,  3 },
    /* WORD_CLOCK_HOUR_SECH    */ {  5,  0,  4 },
    /* WORD_CLOCK_HOUR_SIEB    */ {  5,  6,  4 },
    /* WORD_CLOCK_HOUR_UND     */ {  6, 13,  3 },
    /* WORD_CLOCK_HOUR_20      */ {  7,  5,  7 },
    /* WORD_CLOCK_MIN_UND_UHR  */ {  8,  0,  3 },
    /* WORD_CLOCK_MIN_EIN      */ {  8,  3,  3 },
    /* WORD_CLOCK_MIN_EINE     */ {  8,  3,  4 },
    /* WORD_CLOCK_MIN_2_24H    */ {  8,  7,  4 },
    /* WORD_CLOCK_MIN_3_24H    */ {  8, 11,  4 },
    /* WORD_CLOCK_MIN_4_24H    */ {  9,  0,  4 },
    /* WORD_CLOCK_MIN_5_24H    */ {  9,  5,  4 },
    /* WORD_CLOCK_MIN_6_24H    */ {  9, 10,  5 },
    /* WORD_CLOCK_MIN_7_24H    */ { 10,  0,  6 },
    /* WORD_CLOCK_MIN_8_24H    */ { 10,  6,  4 },
    /* WORD_CLOCK_MIN_9_24H    */ { 10, 10,  4 },
    /* WORD_CLOCK_MIN_10_24H   */ { 11,  0,  4 },
    /* WORD_CLOCK_MIN_11_24H   */ { 11,  4,  3 },
    /* WORD_CLOCK_MIN_12_24H   */ { 11,  8,  5 },
    /* WORD_CLOCK_MIN_SECH    */ {  9, 10,  4 },
    /* WORD_CLOCK_MIN_SIEB    */ { 10,  0,  4 },
    /* WORD_CLOCK_MIN_UND     */ { 11, 13,  3 },
    /* WORD_CLOCK_MIN_20_24H  */ { 12,  0,  7 },
    /* WORD_CLOCK_MIN_30_24H  */ { 13,  0,  7 },
    /* WORD_CLOCK_MIN_40_24H  */ { 13,  8,  7 },
    /* WORD_CLOCK_MIN_50_24H  */ { 14,  0,  7 },
    /* WORD_CLOCK_MIN_MINUTE   */ { 14,  8,  6 },
    /* WORD_CLOCK_MIN_MINUTEN  */ { 14,  8,  7 },
    /* WORD_WORDCLOCK          */ { 12,  7,  9 }
};

/* Index 0 = midnight, 12 = noon; colloquial rows repeat after noon.
 * EIN UHR is grammatical; EINS remains available for FUENF NACH EINS.
 */
static constexpr tWord mcWordHoursTable[HOUR_MODE_MAX_NUMBER][HOURS_COUNT][MAX_HOUR_WORDS] =
{
    {
        { WORD_CLOCK_HOUR_12, WORD_UHR }, { WORD_CLOCK_HOUR_EIN, WORD_UHR },
        { WORD_CLOCK_HOUR_2,  WORD_UHR }, { WORD_CLOCK_HOUR_3,   WORD_UHR },
        { WORD_CLOCK_HOUR_4,  WORD_UHR }, { WORD_CLOCK_HOUR_5,   WORD_UHR },
        { WORD_CLOCK_HOUR_6,  WORD_UHR }, { WORD_CLOCK_HOUR_7,   WORD_UHR },
        { WORD_CLOCK_HOUR_8,  WORD_UHR }, { WORD_CLOCK_HOUR_9,   WORD_UHR },
        { WORD_CLOCK_HOUR_10, WORD_UHR }, { WORD_CLOCK_HOUR_11,  WORD_UHR },
        { WORD_CLOCK_HOUR_12, WORD_UHR }, { WORD_CLOCK_HOUR_EIN, WORD_UHR },
        { WORD_CLOCK_HOUR_2,  WORD_UHR }, { WORD_CLOCK_HOUR_3,   WORD_UHR },
        { WORD_CLOCK_HOUR_4,  WORD_UHR }, { WORD_CLOCK_HOUR_5,   WORD_UHR },
        { WORD_CLOCK_HOUR_6,  WORD_UHR }, { WORD_CLOCK_HOUR_7,   WORD_UHR },
        { WORD_CLOCK_HOUR_8,  WORD_UHR }, { WORD_CLOCK_HOUR_9,   WORD_UHR },
        { WORD_CLOCK_HOUR_10, WORD_UHR }, { WORD_CLOCK_HOUR_11,  WORD_UHR }
    },
    {
        { WORD_CLOCK_HOUR_12 }, { WORD_CLOCK_HOUR_1  },
        { WORD_CLOCK_HOUR_2  }, { WORD_CLOCK_HOUR_3  },
        { WORD_CLOCK_HOUR_4  }, { WORD_CLOCK_HOUR_5  },
        { WORD_CLOCK_HOUR_6  }, { WORD_CLOCK_HOUR_7  },
        { WORD_CLOCK_HOUR_8  }, { WORD_CLOCK_HOUR_9  },
        { WORD_CLOCK_HOUR_10 }, { WORD_CLOCK_HOUR_11 },
        { WORD_CLOCK_HOUR_12 }, { WORD_CLOCK_HOUR_1  },
        { WORD_CLOCK_HOUR_2  }, { WORD_CLOCK_HOUR_3  },
        { WORD_CLOCK_HOUR_4  }, { WORD_CLOCK_HOUR_5  },
        { WORD_CLOCK_HOUR_6  }, { WORD_CLOCK_HOUR_7  },
        { WORD_CLOCK_HOUR_8  }, { WORD_CLOCK_HOUR_9  },
        { WORD_CLOCK_HOUR_10 }, { WORD_CLOCK_HOUR_11 }
    },
    {
        /* 00..12 */
        { WORD_CLOCK_HOUR_0,   WORD_UHR }, { WORD_CLOCK_HOUR_EIN, WORD_UHR },
        { WORD_CLOCK_HOUR_2,   WORD_UHR }, { WORD_CLOCK_HOUR_3,   WORD_UHR },
        { WORD_CLOCK_HOUR_4,   WORD_UHR }, { WORD_CLOCK_HOUR_5,   WORD_UHR },
        { WORD_CLOCK_HOUR_6,   WORD_UHR }, { WORD_CLOCK_HOUR_7,   WORD_UHR },
        { WORD_CLOCK_HOUR_8,   WORD_UHR }, { WORD_CLOCK_HOUR_9,   WORD_UHR },
        { WORD_CLOCK_HOUR_10,  WORD_UHR }, { WORD_CLOCK_HOUR_11,  WORD_UHR },
        { WORD_CLOCK_HOUR_12,  WORD_UHR },
        /* 13..19: shortened stems for sechzehn / siebzehn. */
        { WORD_CLOCK_HOUR_3,    WORD_CLOCK_HOUR_10, WORD_UHR },
        { WORD_CLOCK_HOUR_4,    WORD_CLOCK_HOUR_10, WORD_UHR },
        { WORD_CLOCK_HOUR_5,    WORD_CLOCK_HOUR_10, WORD_UHR },
        { WORD_CLOCK_HOUR_SECH, WORD_CLOCK_HOUR_10, WORD_UHR },
        { WORD_CLOCK_HOUR_SIEB, WORD_CLOCK_HOUR_10, WORD_UHR },
        { WORD_CLOCK_HOUR_8,    WORD_CLOCK_HOUR_10, WORD_UHR },
        { WORD_CLOCK_HOUR_9,    WORD_CLOCK_HOUR_10, WORD_UHR },
        /* 20..23 */
        { WORD_CLOCK_HOUR_20, WORD_UHR },
        { WORD_CLOCK_HOUR_EIN, WORD_CLOCK_HOUR_UND, WORD_CLOCK_HOUR_20, WORD_UHR },
        { WORD_CLOCK_HOUR_2,   WORD_CLOCK_HOUR_UND, WORD_CLOCK_HOUR_20, WORD_UHR },
        { WORD_CLOCK_HOUR_3,   WORD_CLOCK_HOUR_UND, WORD_CLOCK_HOUR_20, WORD_UHR }
    }
};

/* Compile-time generation; no allocation or runtime string processing. */
constexpr std::array<tMinuteDisplay, EXACT_MINUTE_COUNT> MakeExactMinuteTable()
{
    constexpr tWord wUnits[] =
    {
        WORD_END_OF_WORDS, WORD_CLOCK_MIN_EIN, WORD_CLOCK_MIN_2_24H,
        WORD_CLOCK_MIN_3_24H, WORD_CLOCK_MIN_4_24H, WORD_CLOCK_MIN_5_24H,
        WORD_CLOCK_MIN_6_24H, WORD_CLOCK_MIN_7_24H, WORD_CLOCK_MIN_8_24H,
        WORD_CLOCK_MIN_9_24H, WORD_CLOCK_MIN_10_24H,
        WORD_CLOCK_MIN_11_24H, WORD_CLOCK_MIN_12_24H
    };
    constexpr tWord wTens[] =
    {
        WORD_END_OF_WORDS, WORD_CLOCK_MIN_10_24H, WORD_CLOCK_MIN_20_24H,
        WORD_CLOCK_MIN_30_24H, WORD_CLOCK_MIN_40_24H, WORD_CLOCK_MIN_50_24H
    };
    std::array<tMinuteDisplay, EXACT_MINUTE_COUNT> wTable = {};
    for (uint8_t wMinute = 0; wMinute < EXACT_MINUTE_COUNT; ++wMinute)
    {
        tMinuteDisplay& wrDisplay = wTable[wMinute];
        wrDisplay.mHourMode = HOUR_MODE_24H;
        wrDisplay.mFlags = NO_FLAGS;
        if (wMinute == 0) // UHR alone, no UND NULL MINUTEN
        {
            continue;
        }
        uint8_t wIndex = 0;
        wrDisplay.wMinuteWords[wIndex++] = WORD_CLOCK_MIN_UND_UHR;
        if (wMinute <= 12)
        {
            wrDisplay.wMinuteWords[wIndex++] = (wMinute == 1) ? WORD_CLOCK_MIN_EINE : wUnits[wMinute];
        }
        else if (wMinute < 20)
        {
            wrDisplay.wMinuteWords[wIndex++] = (wMinute == 16) ? WORD_CLOCK_MIN_SECH :
                (wMinute == 17) ? WORD_CLOCK_MIN_SIEB : wUnits[wMinute - 10];
            wrDisplay.wMinuteWords[wIndex++] = WORD_CLOCK_MIN_10_24H;
        }
        else
        {
            if ((wMinute % 10) != 0)
            {
                wrDisplay.wMinuteWords[wIndex++] = wUnits[wMinute % 10];
                wrDisplay.wMinuteWords[wIndex++] = WORD_CLOCK_MIN_UND;
            }
            wrDisplay.wMinuteWords[wIndex++] = wTens[wMinute / 10];
        }
        // Dedicated suffix: main and extra minutes must not share LEDs/colors.
        wrDisplay.wMinuteWords[wIndex] = (wMinute == 1) ? WORD_CLOCK_MIN_MINUTE : WORD_CLOCK_MIN_MINUTEN;
    }
    return wTable;
}
static constexpr auto mcWordExactMinutesTable = MakeExactMinuteTable();

/* Existing five-minute wording, including its original :20/:40 distinction. */
static constexpr tMinuteDisplay mcColloquialMinutesTable[2][MINUTE_COUNT] =
{
    {
        { HOUR_MODE_0, NO_FLAGS,      { WORD_GENAU } },
        { HOUR_MODE_1, NO_FLAGS,      { WORD_CLOCK_MIN_5,  WORD_NACH } },
        { HOUR_MODE_1, NO_FLAGS,      { WORD_CLOCK_MIN_10, WORD_NACH } },
        { HOUR_MODE_1, NO_FLAGS,      { WORD_VIERTEL,      WORD_NACH } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_10, WORD_VOR,  WORD_HALB } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_5,  WORD_VOR,  WORD_HALB } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_HALB } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_5,  WORD_NACH, WORD_HALB } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_10, WORD_NACH, WORD_HALB } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_VIERTEL,      WORD_VOR } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_10, WORD_VOR } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_5,  WORD_VOR } }
    },
    {
        { HOUR_MODE_0, NO_FLAGS,      { WORD_GENAU } },
        { HOUR_MODE_1, NO_FLAGS,      { WORD_CLOCK_MIN_5,  WORD_NACH } },
        { HOUR_MODE_1, NO_FLAGS,      { WORD_CLOCK_MIN_10, WORD_NACH } },
        { HOUR_MODE_1, NO_FLAGS,      { WORD_VIERTEL,      WORD_NACH } },
        { HOUR_MODE_1, NO_FLAGS,      { WORD_CLOCK_MIN_20, WORD_NACH } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_5,  WORD_VOR,  WORD_HALB } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_HALB } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_5,  WORD_NACH, WORD_HALB } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_20, WORD_VOR } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_VIERTEL,      WORD_VOR } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_10, WORD_VOR } },
        { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_CLOCK_MIN_5,  WORD_VOR } }
    }
};

constexpr std::array<std::array<tMinuteDisplay, MINUTE_COUNT>, WORDCLOCK_MODE_NUMBER> MakeMinuteTable()
{
    std::array<std::array<tMinuteDisplay, MINUTE_COUNT>, WORDCLOCK_MODE_NUMBER> wTable = {};
    for (uint8_t wStep = 0; wStep < MINUTE_COUNT; ++wStep)
    {
        wTable[WORDCLOCK_MODE_0][wStep] = mcColloquialMinutesTable[0][wStep];
        wTable[WORDCLOCK_MODE_1][wStep] = mcColloquialMinutesTable[1][wStep];
        wTable[WORDCLOCK_MODE_VIERTEL][wStep] = mcColloquialMinutesTable[1][wStep];
        wTable[WORDCLOCK_MODE_DREIVIERTEL][wStep] = mcColloquialMinutesTable[1][wStep];
        for (tWordClockMode wMode : { WORDCLOCK_MODE_24H, WORDCLOCK_MODE_24H_EXACT,
                                     WORDCLOCK_MODE_12H, WORDCLOCK_MODE_12H_EXACT })
        {
            wTable[wMode][wStep] = mcWordExactMinutesTable[wStep * 5];
            if (wMode == WORDCLOCK_MODE_12H || wMode == WORDCLOCK_MODE_12H_EXACT)
            {
                wTable[wMode][wStep].mHourMode = HOUR_MODE_0;
            }
        }
    }
    wTable[WORDCLOCK_MODE_VIERTEL][3] = { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_VIERTEL } };
    wTable[WORDCLOCK_MODE_VIERTEL][9] = { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_DREIVIERTEL } };
    wTable[WORDCLOCK_MODE_DREIVIERTEL][9] = { HOUR_MODE_1, HOUR_OFFSET_1, { WORD_DREIVIERTEL } };
    return wTable;
}
/* Exact-mode rows contain only five-minute samples; use the accessor below. */
static constexpr auto mcWordMinutesTable = MakeMinuteTable();

static constexpr tWord mcWordExtraMinutesTable[EXTRA_MINUTE_COUNT][MAX_EXTRA_MINUTE_WORDS] =
{
    { WORD_END_OF_WORDS },
    { WORD_PLUS, WORD_NUM_1, WORD_MINUTE  },
    { WORD_PLUS, WORD_NUM_2, WORD_MINUTEN },
    { WORD_PLUS, WORD_NUM_3, WORD_MINUTEN },
    { WORD_PLUS, WORD_NUM_4, WORD_MINUTEN }
};

/* Inputs: valid enum mode, civil hour 0..23, minute 0..59. */
constexpr bool IsLayoutExactMode(tWordClockMode aMode)
{
    return aMode == WORDCLOCK_MODE_24H_EXACT || aMode == WORDCLOCK_MODE_12H_EXACT;
}

constexpr tMinuteDisplay GetLayoutMinuteDisplay(tWordClockMode aMode, uint8_t aMinute)
{
    tMinuteDisplay wDisplay = IsLayoutExactMode(aMode) ? mcWordExactMinutesTable[aMinute] :
        mcWordMinutesTable[aMode][aMinute / 5];
    if (aMode == WORDCLOCK_MODE_12H_EXACT)
    {
        wDisplay.mHourMode = HOUR_MODE_0;
    }
    return wDisplay;
}

constexpr uint8_t GetLayoutHourIndex(uint8_t aHour, const tMinuteDisplay& arMinute)
{
    return (aHour + ((arMinute.mFlags & HOUR_OFFSET_1) ? 1 : 0)) % HOURS_COUNT;
}

constexpr uint8_t GetLayoutExtraMinuteIndex(tWordClockMode aMode, uint8_t aMinute)
{
    return IsLayoutExactMode(aMode) ? 0 : aMinute % 5;
}

/* Structural checks, evaluated when this proposal is eventually compiled.
 * ID/bounds checks are not a substitute for spelling/physical-panel review.
 */
constexpr bool ValidateLayout24h()
{
    for (uint8_t wRow = 0; wRow < MATRIX_HEIGHT; ++wRow)
    {
        unsigned wLength = 0;
        while (mDisplayLayout[wRow][wLength] != '\0')
        {
            ++wLength;
        }
        if (wLength != MATRIX_WIDTH * 3 + 2) // Two trailing spaces, as in the existing panel
        {
            return false;
        }
    }
    for (unsigned wWord = 1; wWord < WORD_MAX_NUMBER; ++wWord)
    {
        const tLayoutWordData24h& wrWord = mcWordDataArray[wWord];
        if (wrWord.mRow >= MATRIX_HEIGHT || wrWord.mLength == 0 ||
            wrWord.mColumn + wrWord.mLength > MATRIX_WIDTH)
        {
            return false;
        }
    }
    for (uint8_t wMode = 0; wMode < WORDCLOCK_MODE_NUMBER; ++wMode)
    {
        for (uint8_t wHour = 0; wHour < HOURS_COUNT; ++wHour)
        {
            for (uint8_t wMinute = 0; wMinute < EXACT_MINUTE_COUNT; ++wMinute)
            {
                const auto wModeId = static_cast<tWordClockMode>(wMode);
                const tMinuteDisplay wDisplay = GetLayoutMinuteDisplay(wModeId, wMinute);
                if (wDisplay.mHourMode >= HOUR_MODE_MAX_NUMBER ||
                    GetLayoutExtraMinuteIndex(wModeId, wMinute) >= EXTRA_MINUTE_COUNT)
                {
                    return false;
                }
                for (tWord wWord : wDisplay.wMinuteWords)
                {
                    if (wWord >= WORD_MAX_NUMBER)
                    {
                        return false;
                    }
                }
                for (tWord wWord : mcWordHoursTable[wDisplay.mHourMode][GetLayoutHourIndex(wHour, wDisplay)])
                {
                    if (wWord >= WORD_MAX_NUMBER)
                    {
                        return false;
                    }
                }
                for (tWord wWord : mcWordExtraMinutesTable[GetLayoutExtraMinuteIndex(wModeId, wMinute)])
                {
                    if (wWord >= WORD_MAX_NUMBER)
                    {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

static_assert(WORD_MAX_NUMBER <= 256, "Word IDs must fit tWord (uint8_t)");
static_assert(ValidateLayout24h(), "Invalid German 24h layout or table");
static_assert(GetLayoutHourIndex(12, GetLayoutMinuteDisplay(WORDCLOCK_MODE_24H_EXACT, 0)) == 12,
    "Noon must not become midnight");
static_assert(GetLayoutHourIndex(23, GetLayoutMinuteDisplay(WORDCLOCK_MODE_1, 55)) == 0,
    "Colloquial next-hour wording must wrap at midnight");
static_assert(mcWordExactMinutesTable[1].wMinuteWords[1] == WORD_CLOCK_MIN_EINE,
    "Use eine Minute");
static_assert(mcWordExactMinutesTable[16].wMinuteWords[1] == WORD_CLOCK_MIN_SECH &&
    mcWordExactMinutesTable[17].wMinuteWords[1] == WORD_CLOCK_MIN_SIEB,
    "Use shortened stems for sechzehn and siebzehn");
static_assert(mcWordExactMinutesTable[59].wMinuteWords[4] == WORD_CLOCK_MIN_MINUTEN,
    "The longest minute expression must fit five slots");