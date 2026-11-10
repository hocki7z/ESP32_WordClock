/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * DisplayLayout_de_24h_2.h -- German 18x18 front-panel proposal, 10.11.2026
 *
 * Layout and coordinates adapted from wordclock24h/make-tables/tables24h-de
 * Copyright (c) 2018-2019 Frank Meyer - frank(at)fli4l.de.
 * This file is free software under the GNU General Public License, version 2
 * or (at your option) any later version; without any warranty. See
 * https://www.gnu.org/licenses/old-licenses/gpl-2.0.html.
 * The reference's GPL attribution applies to this additional derived header;
 * this does not change the license declarations of other project files.
 *
 * PREPARATION ONLY: not included or used by the application.
 * Self-contained C++17; select INSTEAD OF the other layout headers, not beside
 * them (the public layout names deliberately follow the same convention).
 * No dependency on the separate wordclock24h checkout at build/runtime.
 *
 * Retains the reference's original 18-column, 16-row word positions. Row 16
 * supplies WORDCLOCK for startup; row 17 is dedicated to +1..4 MINUTE(N).
 * Every cell outside the declared vocabulary is '#', including removed
 * temperature/Jester-only vocabulary. Overlapping word spans are intentional.
 * 'u'/'o' represent single-cell German umlauts, as in the current preview.
 * DREISSIG retains the reference's TWO S cells; do not substitute one sharp-s.
 *
 * Modes 0/1 retain the current Wessi/Rhein-Ruhr relative-minute conventions.
 * Their full-hour display uses EIN UHR (no GENAU: absent from this panel).
 * The remaining 24 modes implement the reference's ordinary clock families:
 * numeric 12/24h and six regional families, with optional minute units and
 * day-period suffixes. The reference's Oesi and Tirol wordings coincide.
 * Countdown, Jester and temperature are not implemented here.
 * Numeric 12h midnight is ZWOELF UHR, correcting the reference HM_4 NULL UHR.
 * Source-derived bare regional modes retain EINE MINUTE when the relative
 * number is one. Legacy IDs 0/1 use EINS in their added exact-minute extension.
 * ES IST remains independently selectable by the future scene owner; this
 * draft does not import the source renderer's forced :00/:30 visibility rule.
 *
 * Default sampling is floor-to-five, with +1 Minute / +2..4 Minuten in an
 * independent group. EXACT_MINUTES instead spells out each minute and returns
 * no remainder. Sampling applies before hour offsets and EIN/EINS selection.
 * Day-period wording uses the offset-adjusted civil hour, including rollover.
 *
 * Future integration (NOT done here):
 * - Use GetLayoutPresentation(), or the minute/hour/extra accessors below.
 *   The active renderer's /5 lookup and 12h normalization are incompatible.
 * - Increase WordGroup capacity BEFORE its declaration to at least
 *   LAYOUT_MAX_TIME_WORDS; this proposal reserves 5 minute + 6 hour slots.
 * - Adapt explicit tWordData references to tLayoutWordData24h2, or consolidate
 *   the shared coordinate type first; matching fields are not a conversion.
 * - Wire selected mode/resolution and update settings/UI bounds.
 * - Update the preview dimensions and translate the u/o placeholders.
 * - Do not select or illuminate '#' as a printable/displayable character.
 *
 * Usage examples (optional ES IST omitted, default five-minute resolution):
 * - 13:21, Wessi: ZEHN VOR HALB ZWEI +1 MINUTE.
 * - 13:21, numeric 24h: DREIZEHN UHR UND ZWANZIG MINUTEN +1 MINUTE.
 * - Same numeric mode with EXACT_MINUTES: DREIZEHN UHR UND
 *   EINUNDZWANZIG MINUTEN, with no extra-minute words.
 * - 04:00, day-period numeric 12h: VIER UHR FRUEH MORGENS.
 *
 * Regional conventions (:15 / :20 / :40 / :45):
 * Ossi:     VIERTEL / ZEHN VOR HALB / ZEHN NACH HALB / DREIVIERTEL.
 * Wessi:    VIERTEL NACH / ZEHN VOR HALB / ZEHN NACH HALB / VIERTEL VOR.
 * Schwaben: VIERTEL / ZWANZIG NACH / ZWANZIG VOR / DREIVIERTEL.
 * Rhein:    VIERTEL NACH / ZWANZIG NACH / ZWANZIG VOR / VIERTEL VOR.
 * Oesi and Tirol: same as Rhein, but DREIVIERTEL at :45.
 * MINUTEN variants add units to numeric relative-minute phrases, not to
 * VIERTEL/HALB. Bare regional variants still retain singular EINE MINUTE.
 */
#pragma once

#include <array>
#include <cstddef>
#include <stdint.h>

using tWord = uint8_t;

#define MATRIX_WIDTH                18
#define MATRIX_HEIGHT               18
#define MATRIX_SIZE                 (MATRIX_WIDTH * MATRIX_HEIGHT)
#define HOURS_COUNT                 24
#define MINUTE_COUNT                60  // Full-minute tables, NOT twelve /5 entries
#define EXTRA_MINUTE_COUNT          5
#define MAX_MINUTE_WORDS            5
#define MAX_HOUR_WORDS              6
#define MAX_EXTRA_MINUTE_WORDS      3
#define LAYOUT_MAX_TIME_WORDS       (MAX_MINUTE_WORDS + MAX_HOUR_WORDS)
#define NO_FLAGS                    0x00
#define HOUR_OFFSET_1               0x01  // Project flag, NOT upstream MDF_IT_IS_1

/** @brief Zero-based logical span; length counts LEDs, not encoded bytes. */
struct tLayoutWordData24h2
{
    uint8_t mRow;
    uint8_t mColumn;
    uint8_t mLength;
};

/* Authoritative one-byte-per-cell transcription. Only used letters remain. */
static constexpr const char* mcLayoutCells[MATRIX_HEIGHT] =
{
    /*             012345678901234567 */
    /* 00 */      "ES#IST#VIERTELEINS",
    /* 01 */      "DREINE#SECHSIEBEN#",
    /* 02 */      "ELFuNFNEUNVIERACHT",
    /* 03 */      "NULLZWEI#ZWoLFZEHN",
    /* 04 */      "UND#ZWANZIGVIERZIG",
    /* 05 */      "DREISSIGFuNFZIGUHR",
    /* 06 */      "MINUTEN#VORUNDNACH",
    /* 07 */      "###DREIVIERTELHALB",
    /* 08 */      "SIEBENEUNULLZWEINE",
    /* 09 */      "FuNFSECHS#ACHTVIER",
    /* 10 */      "DREINSUND#ELF#ZEHN",
    /* 11 */      "ZWANZIG###DREISSIG",
    /* 12 */      "VIERZIGZWoLFuNFZIG",
    /* 13 */      "MINUTENUHR#FRuHVOR",
    /* 14 */      "ABENDS######NACHTS",
    /* 15 */      "MORGENS####MITTAGS",
    /* 16 */      "WORDCLOCK#########",
    /* 17 */      "+1234#MINUTEN#####"
};

/* Emit the current preview encoding: glyph at column*3+2, two trailing spaces.
 * Compact strings above avoid visually ambiguous groups of padding spaces.
 */
constexpr auto MakeEncodedLayout24h2()
{
    std::array<std::array<char, MATRIX_WIDTH * 3 + 3>, MATRIX_HEIGHT> wRows = {};
    for (unsigned wRow = 0; wRow < MATRIX_HEIGHT; ++wRow)
    {
        for (unsigned wIndex = 0; wIndex < MATRIX_WIDTH * 3 + 2; ++wIndex)
        {
            wRows[wRow][wIndex] = ' ';
        }
        for (unsigned wColumn = 0; wColumn < MATRIX_WIDTH; ++wColumn)
        {
            wRows[wRow][wColumn * 3 + 2] = mcLayoutCells[wRow][wColumn];
        }
    }
    return wRows;
}
static constexpr auto mcEncodedLayout24h2 = MakeEncodedLayout24h2();
static constexpr const char* mDisplayLayout[MATRIX_HEIGHT] =
{
    mcEncodedLayout24h2[ 0].data(), mcEncodedLayout24h2[ 1].data(),
    mcEncodedLayout24h2[ 2].data(), mcEncodedLayout24h2[ 3].data(),
    mcEncodedLayout24h2[ 4].data(), mcEncodedLayout24h2[ 5].data(),
    mcEncodedLayout24h2[ 6].data(), mcEncodedLayout24h2[ 7].data(),
    mcEncodedLayout24h2[ 8].data(), mcEncodedLayout24h2[ 9].data(),
    mcEncodedLayout24h2[10].data(), mcEncodedLayout24h2[11].data(),
    mcEncodedLayout24h2[12].data(), mcEncodedLayout24h2[13].data(),
    mcEncodedLayout24h2[14].data(), mcEncodedLayout24h2[15].data(),
    mcEncodedLayout24h2[16].data(), mcEncodedLayout24h2[17].data()
};

/* One definition supplies enum order, coordinates, lengths and spellings.
 * Suffix _1 = upper number block; _2 = lower number block (source convention).
 * Unused source aliases IN/EINER/NACH_2 and temperature-only words are omitted.
 */
#define WC18_WORDS(X) \
    X(WORD_ES,          0,  0, "ES") \
    X(WORD_IST,         0,  3, "IST") \
    X(WORD_VIERTEL_1,   0,  7, "VIERTEL") \
    X(WORD_EIN_1,       0, 14, "EIN") \
    X(WORD_EINS_1,      0, 14, "EINS") \
    X(WORD_DREI_1,      1,  0, "DREI") \
    X(WORD_EINE_1,      1,  2, "EINE") \
    X(WORD_SECH_1,      1,  7, "SECH") \
    X(WORD_SECHS_1,     1,  7, "SECHS") \
    X(WORD_SIEB_1,      1, 11, "SIEB") \
    X(WORD_SIEBEN_1,    1, 11, "SIEBEN") \
    X(WORD_ELF_1,       2,  0, "ELF") \
    X(WORD_FUENF_1,     2,  2, "FuNF") \
    X(WORD_NEUN_1,      2,  6, "NEUN") \
    X(WORD_VIER_1,      2, 10, "VIER") \
    X(WORD_ACHT_1,      2, 14, "ACHT") \
    X(WORD_NULL_1,      3,  0, "NULL") \
    X(WORD_ZWEI_1,      3,  4, "ZWEI") \
    X(WORD_ZWOELF_1,    3,  9, "ZWoLF") \
    X(WORD_ZEHN_1,      3, 14, "ZEHN") \
    X(WORD_UND_1,       4,  0, "UND") \
    X(WORD_ZWANZIG_1,   4,  4, "ZWANZIG") \
    X(WORD_VIERZIG_1,   4, 11, "VIERZIG") \
    X(WORD_DREISSIG_1,  5,  0, "DREISSIG") \
    X(WORD_FUENFZIG_1,  5,  8, "FuNFZIG") \
    X(WORD_UHR_1,       5, 15, "UHR") \
    X(WORD_MINUTE_1,    6,  0, "MINUTE") \
    X(WORD_MINUTEN_1,   6,  0, "MINUTEN") \
    X(WORD_VOR_1,       6,  8, "VOR") \
    X(WORD_UND_2,       6, 11, "UND") \
    X(WORD_NACH_1,      6, 14, "NACH") \
    X(WORD_DREIVIERTEL, 7,  3, "DREIVIERTEL") \
    X(WORD_HALB,        7, 14, "HALB") \
    X(WORD_SIEB_2,      8,  0, "SIEB") \
    X(WORD_SIEBEN_2,    8,  0, "SIEBEN") \
    X(WORD_NEUN_2,      8,  5, "NEUN") \
    X(WORD_NULL_2,      8,  8, "NULL") \
    X(WORD_ZWEI_2,      8, 12, "ZWEI") \
    X(WORD_EIN_4,       8, 14, "EIN") \
    X(WORD_EINE_2,      8, 14, "EINE") \
    X(WORD_FUENF_2,     9,  0, "FuNF") \
    X(WORD_SECH_2,      9,  4, "SECH") \
    X(WORD_SECHS_2,     9,  4, "SECHS") \
    X(WORD_ACHT_2,      9, 10, "ACHT") \
    X(WORD_VIER_2,      9, 14, "VIER") \
    X(WORD_DREI_2,     10,  0, "DREI") \
    X(WORD_EINS_2,     10,  2, "EINS") \
    X(WORD_UND_3,      10,  6, "UND") \
    X(WORD_ELF_2,      10, 10, "ELF") \
    X(WORD_ZEHN_2,     10, 14, "ZEHN") \
    X(WORD_ZWANZIG_2,  11,  0, "ZWANZIG") \
    X(WORD_DREISSIG_2, 11, 10, "DREISSIG") \
    X(WORD_VIERZIG_2,  12,  0, "VIERZIG") \
    X(WORD_ZWOELF_2,   12,  7, "ZWoLF") \
    X(WORD_FUENFZIG_2, 12, 11, "FuNFZIG") \
    X(WORD_MINUTE_2,   13,  0, "MINUTE") \
    X(WORD_MINUTEN_2,  13,  0, "MINUTEN") \
    X(WORD_UHR_2,      13,  7, "UHR") \
    X(WORD_FRUEH,      13, 11, "FRuH") \
    X(WORD_VOR_2,      13, 15, "VOR") \
    X(WORD_ABENDS,     14,  0, "ABENDS") \
    X(WORD_NACH_3,     14, 12, "NACH") \
    X(WORD_NACHTS,     14, 12, "NACHTS") \
    X(WORD_MORGENS,    15,  0, "MORGENS") \
    X(WORD_MITTAGS,    15, 11, "MITTAGS") \
    X(WORD_WORDCLOCK,  16,  0, "WORDCLOCK") \
    X(WORD_PLUS,       17,  0, "+") \
    X(WORD_NUM_1,      17,  1, "1") \
    X(WORD_NUM_2,      17,  2, "2") \
    X(WORD_NUM_3,      17,  3, "3") \
    X(WORD_NUM_4,      17,  4, "4") \
    X(WORD_MINUTE,     17,  6, "MINUTE") \
    X(WORD_MINUTEN,    17,  6, "MINUTEN")

/** @brief Byte-sized IDs generated in the same order as the coordinate table.
 * Zero is an empty slot; WORD_MAX_NUMBER is a count/overflow marker only.
 */
enum Word : tWord
{
    WORD_END_OF_WORDS = 0,
#define WC18_ENUM(aName, aRow, aColumn, aText) aName,
    WC18_WORDS(WC18_ENUM)
#undef WC18_ENUM
    WORD_MAX_NUMBER
};
/** @brief Word-ID lookup for logical spans, including a zero-length sentinel. */
static constexpr tLayoutWordData24h2 mcWordDataArray[WORD_MAX_NUMBER] =
{
    { 0, 0, 0 },
#define WC18_COORD(aName, aRow, aColumn, aText) { aRow, aColumn, sizeof(aText) - 1 },
    WC18_WORDS(WC18_COORD)
#undef WC18_COORD
};
/* Expected single-byte glyphs for spelling/coverage validation, not phrases. */
static constexpr const char* mcWordSpellings[WORD_MAX_NUMBER] =
{
    "",
#define WC18_TEXT(aName, aRow, aColumn, aText) aText,
    WC18_WORDS(WC18_TEXT)
#undef WC18_TEXT
};
#undef WC18_WORDS

/* Upper hours precede the lower numeric minute block. Lower hours follow
 * upper relative-minute phrases. DAY appends the reference's period words.
 */
enum tHourMode
{
    HOUR_MODE_UPPER_12,
    HOUR_MODE_UPPER_24,
    HOUR_MODE_LOWER_12,
    HOUR_MODE_LOWER_12_DAY,
    HOUR_MODE_LOWER_24,
    HOUR_MODE_MAX_NUMBER
};
/* BARE = HH UHR MM; UNITS = HH UHR UND MM MINUTE(N);
 * AFTER = MM MINUTE(N) NACH HH UHR; REGIONAL = relative/quarter-hour wording.
 */
enum tMinuteStyle24h2 { NUMERIC_BARE, NUMERIC_UNITS, MINUTES_AFTER, REGIONAL };
enum tRegion24h2 { REGION_OSSI, REGION_OESI, REGION_RHEIN_RUHR, REGION_SCHWABEN, REGION_WESSI, REGION_TIROL };
/* Independent of presentation mode: exact wording or stepped base + extras. */
enum tMinuteResolution24h2 { FIVE_MINUTES, EXACT_MINUTES };

/* IDs 0/1 are legacy; six numeric combinations and six regional triplets
 * follow. Triplet suffixes: bare (12-), minute units (12), day period (12+).
 * These IDs are NOT the source project's original tbl_modes ordinals.
 */
enum tWordClockMode
{
    WORDCLOCK_MODE_0 = 0, WORDCLOCK_MODE_1, // Current Wessi / Rhein-Ruhr IDs
    WORDCLOCK_MODE_12H_BARE, WORDCLOCK_MODE_12H, WORDCLOCK_MODE_12H_DAY,
    WORDCLOCK_MODE_24H_BARE, WORDCLOCK_MODE_24H, WORDCLOCK_MODE_24H_AFTER,
    WORDCLOCK_MODE_OSSI, WORDCLOCK_MODE_OSSI_MINUTES, WORDCLOCK_MODE_OSSI_DAY,
    WORDCLOCK_MODE_OESI, WORDCLOCK_MODE_OESI_MINUTES, WORDCLOCK_MODE_OESI_DAY,
    WORDCLOCK_MODE_RHEIN_RUHR, WORDCLOCK_MODE_RHEIN_RUHR_MINUTES, WORDCLOCK_MODE_RHEIN_RUHR_DAY,
    WORDCLOCK_MODE_SCHWABEN, WORDCLOCK_MODE_SCHWABEN_MINUTES, WORDCLOCK_MODE_SCHWABEN_DAY,
    WORDCLOCK_MODE_WESSI, WORDCLOCK_MODE_WESSI_MINUTES, WORDCLOCK_MODE_WESSI_DAY,
    WORDCLOCK_MODE_TIROL, WORDCLOCK_MODE_TIROL_MINUTES, WORDCLOCK_MODE_TIROL_DAY,
    WORDCLOCK_MODE_NUMBER
};
static constexpr const char* mcClockModeItems[WORDCLOCK_MODE_NUMBER] =
{
    "Wessi", "Rhein-Ruhr",
    "hh mm (12-)", "hh mm (12)", "hh mm (12+)",
    "hh mm (24-)", "hh mm (24)", "hh mm (24+)",
    "Ossi/Berlin (12-)", "Ossi/Berlin (12)", "Ossi/Berlin (12+)",
    "Oesi (12-)", "Oesi (12)", "Oesi (12+)",
    "Rhein/Ruhr (12-)", "Rhein/Ruhr (12)", "Rhein/Ruhr (12+)",
    "Schwaben (12-)", "Schwaben (12)", "Schwaben (12+)",
    "Wessi (12-)", "Wessi (12)", "Wessi (12+)",
    "Tirol (12-)", "Tirol (12)", "Tirol (12+)"
};
static constexpr tWordClockMode mcDefaultDisplayClockMode = WORDCLOCK_MODE_24H;
static constexpr tMinuteResolution24h2 mcDefaultMinuteResolution = FIVE_MINUTES;

/** @brief Descriptor separating hour placement, minute style and region.
 * mMinuteUnits requests MINUTE(N); mBareSingularUnit preserves the source's
 * EINE MINUTE exception even when other numeric phrases omit units.
 */
struct tModeConfig24h2
{
    tHourMode mHourMode;
    tMinuteStyle24h2 mStyle;
    tRegion24h2 mRegion;
    bool mMinuteUnits;
    bool mBareSingularUnit = true; // Source MM_13..MM_18 keep EINE MINUTE
};
/** @brief Generate descriptors; regional enum order matches the six triplets. */
constexpr auto MakeModeConfigs24h2()
{
    std::array<tModeConfig24h2, WORDCLOCK_MODE_NUMBER> wModes = {};
    wModes[0] = { HOUR_MODE_LOWER_12, REGIONAL, REGION_WESSI, false, false };
    wModes[1] = { HOUR_MODE_LOWER_12, REGIONAL, REGION_RHEIN_RUHR, false, false };
    wModes[2] = { HOUR_MODE_UPPER_12, NUMERIC_BARE, REGION_WESSI, false };
    wModes[3] = { HOUR_MODE_UPPER_12, NUMERIC_UNITS, REGION_WESSI, true };
    wModes[4] = { HOUR_MODE_LOWER_12_DAY, MINUTES_AFTER, REGION_WESSI, true };
    wModes[5] = { HOUR_MODE_UPPER_24, NUMERIC_BARE, REGION_WESSI, false };
    wModes[6] = { HOUR_MODE_UPPER_24, NUMERIC_UNITS, REGION_WESSI, true };
    wModes[7] = { HOUR_MODE_LOWER_24, MINUTES_AFTER, REGION_WESSI, true };
    for (unsigned wRegion = 0; wRegion < 6; ++wRegion)
    {
        for (unsigned wVariant = 0; wVariant < 3; ++wVariant)
        {
            wModes[8 + wRegion * 3 + wVariant] =
            {
                wVariant == 2 ? HOUR_MODE_LOWER_12_DAY : HOUR_MODE_LOWER_12,
                REGIONAL, static_cast<tRegion24h2>(wRegion), wVariant != 0
            };
        }
    }
    return wModes;
}
static constexpr auto mcModeConfigs24h2 = MakeModeConfigs24h2();

/* A bounded, zero-padded word list. Overflow marks the last slot invalid so
 * compile-time validation fails instead of silently truncating a phrase.
 */
template <std::size_t N>
constexpr void AppendLayoutWord(std::array<tWord, N>& arWords, tWord aWord)
{
    if (aWord == WORD_END_OF_WORDS)
    {
        return;
    }
    for (unsigned wIndex = 0; wIndex < N; ++wIndex)
    {
        if (arWords[wIndex] == WORD_END_OF_WORDS)
        {
            arWords[wIndex] = aWord;
            return;
        }
    }
    arWords[N - 1] = WORD_MAX_NUMBER;
}

/* EIN for compounds/UHR, EINS standalone, EINE before singular MINUTE. */
enum tOneForm24h2 { ONE_EIN, ONE_EINS, ONE_EINE };
/** @brief Physical vocabulary for one number block, including grammar stems. */
struct tNumberBlock24h2
{
    tWord mUnits[13];
    tWord mTens[6];
    tWord mEin, mEins, mEine, mSech, mSieb, mUnd;
};
static constexpr tNumberBlock24h2 mcNumberBlocks24h2[2] =
{
    {
        { WORD_NULL_1, WORD_EIN_1, WORD_ZWEI_1, WORD_DREI_1, WORD_VIER_1,
          WORD_FUENF_1, WORD_SECHS_1, WORD_SIEBEN_1, WORD_ACHT_1, WORD_NEUN_1,
          WORD_ZEHN_1, WORD_ELF_1, WORD_ZWOELF_1 },
        { 0, WORD_ZEHN_1, WORD_ZWANZIG_1, WORD_DREISSIG_1, WORD_VIERZIG_1, WORD_FUENFZIG_1 },
        WORD_EIN_1, WORD_EINS_1, WORD_EINE_1, WORD_SECH_1, WORD_SIEB_1, WORD_UND_1
    },
    {
        { WORD_NULL_2, WORD_EIN_4, WORD_ZWEI_2, WORD_DREI_2, WORD_VIER_2,
          WORD_FUENF_2, WORD_SECHS_2, WORD_SIEBEN_2, WORD_ACHT_2, WORD_NEUN_2,
          WORD_ZEHN_2, WORD_ELF_2, WORD_ZWOELF_2 },
        { 0, WORD_ZEHN_2, WORD_ZWANZIG_2, WORD_DREISSIG_2, WORD_VIERZIG_2, WORD_FUENFZIG_2 },
        WORD_EIN_4, WORD_EINS_2, WORD_EINE_2, WORD_SECH_2, WORD_SIEB_2, WORD_UND_3
    }
};

/** @brief Append a German number using one of the two physical number blocks.
 * @param arWords Zero-padded output list; invalid marker signals overflow.
 * @param aNumber Number in 0..59; caller must enforce this range.
 * @param aLower Select lower (true) or upper (false) printed number block.
 * @param aOneForm Grammar for the standalone number one; compounds use EIN.
 */
template <std::size_t N>
constexpr void AppendLayoutNumber(std::array<tWord, N>& arWords, uint8_t aNumber,
    bool aLower, tOneForm24h2 aOneForm)
{
    const auto& wrBlock = mcNumberBlocks24h2[aLower ? 1 : 0];
    if (aNumber <= 12)
    {
        AppendLayoutWord(arWords, aNumber != 1 ? wrBlock.mUnits[aNumber] :
            aOneForm == ONE_EINE ? wrBlock.mEine : aOneForm == ONE_EINS ? wrBlock.mEins : wrBlock.mEin);
    }
    else if (aNumber < 20)
    {
        AppendLayoutWord(arWords, aNumber == 16 ? wrBlock.mSech :
            aNumber == 17 ? wrBlock.mSieb : wrBlock.mUnits[aNumber - 10]);
        AppendLayoutWord(arWords, wrBlock.mUnits[10]);
    }
    else
    {
        if (aNumber % 10 != 0)
        {
            AppendLayoutWord(arWords, wrBlock.mUnits[aNumber % 10]);
            AppendLayoutWord(arWords, wrBlock.mUnd);
        }
        AppendLayoutWord(arWords, wrBlock.mTens[aNumber / 10]);
    }
}

/** @brief Build [hour mode][civil hour 0..23][word slot], excluding ES IST.
 * Twelve-hour rows repeat number names after noon; DAY adds the appropriate
 * period. EIN/EINS for bare full hours is resolved by GetLayoutHourWords().
 */
constexpr auto MakeHourTable24h2()
{
    std::array<std::array<std::array<tWord, MAX_HOUR_WORDS>, HOURS_COUNT>, HOUR_MODE_MAX_NUMBER> wTable = {};
    for (unsigned wMode = 0; wMode < HOUR_MODE_MAX_NUMBER; ++wMode)
    {
        for (uint8_t wHour = 0; wHour < HOURS_COUNT; ++wHour)
        {
            auto& wrWords = wTable[wMode][wHour];
            const bool wLower = wMode >= HOUR_MODE_LOWER_12;
            const bool w24h = wMode == HOUR_MODE_UPPER_24 || wMode == HOUR_MODE_LOWER_24;
            const uint8_t wNumber = w24h ? wHour : (wHour % 12 == 0 ? 12 : wHour % 12);
            AppendLayoutNumber(wrWords, wNumber, wLower, wMode == HOUR_MODE_LOWER_12 ? ONE_EINS : ONE_EIN);
            if (wMode != HOUR_MODE_LOWER_12)
            {
                AppendLayoutWord(wrWords, wLower ? WORD_UHR_2 : WORD_UHR_1);
            }
            if (wMode == HOUR_MODE_LOWER_12_DAY)
            {
                if (wHour <= 3 || wHour == 23)
                {
                    AppendLayoutWord(wrWords, WORD_NACHTS);
                }
                else if (wHour <= 9)
                {
                    if (wHour <= 5) AppendLayoutWord(wrWords, WORD_FRUEH);
                    AppendLayoutWord(wrWords, WORD_MORGENS);
                }
                else if (wHour <= 17)
                {
                    if (wHour <= 11) AppendLayoutWord(wrWords, WORD_VOR_2);
                    if (wHour >= 14) AppendLayoutWord(wrWords, WORD_NACH_3);
                    AppendLayoutWord(wrWords, WORD_MITTAGS);
                }
                else
                {
                    AppendLayoutWord(wrWords, WORD_ABENDS);
                }
            }
        }
    }
    return wTable;
}
static constexpr auto mcWordHoursTable = MakeHourTable24h2();

/** @brief Resolved minute-word IDs plus the referenced hour mode and offset.
 * Five zero-padded slots hold even the longest numeric or half-hour phrase.
 */
struct tMinuteDisplay
{
    tHourMode mHourMode;
    uint8_t mFlags;
    std::array<tWord, MAX_MINUTE_WORDS> wMinuteWords;
};

/** @brief Generate one full-minute entry from a valid mode descriptor.
 * @param aMinute Minute in 0..59, already sampled if stepped output is wanted.
 * Numeric modes never advance the hour; relative modes may refer to the next.
 */
constexpr tMinuteDisplay MakeMinuteDisplay24h2(const tModeConfig24h2& arConfig, uint8_t aMinute)
{
    tMinuteDisplay wDisplay = { arConfig.mHourMode, NO_FLAGS, {} };
    auto& wrWords = wDisplay.wMinuteWords;
    if (aMinute == 0)
    {
        if (arConfig.mHourMode == HOUR_MODE_LOWER_12) AppendLayoutWord(wrWords, WORD_UHR_2);
        return wDisplay;
    }
    if (arConfig.mStyle != REGIONAL)
    {
        const bool wLower = arConfig.mStyle != MINUTES_AFTER;
        if (arConfig.mStyle == NUMERIC_UNITS) AppendLayoutWord(wrWords, WORD_UND_2);
        AppendLayoutNumber(wrWords, aMinute, wLower, arConfig.mMinuteUnits ? ONE_EINE : ONE_EINS);
        if (arConfig.mMinuteUnits)
        {
            AppendLayoutWord(wrWords, wLower ? (aMinute == 1 ? WORD_MINUTE_2 : WORD_MINUTEN_2) :
                (aMinute == 1 ? WORD_MINUTE_1 : WORD_MINUTEN_1));
        }
        if (arConfig.mStyle == MINUTES_AFTER) AppendLayoutWord(wrWords, WORD_NACH_1);
        return wDisplay;
    }
    const auto wRegion = arConfig.mRegion;
    const bool wQuarterNext = wRegion == REGION_OSSI || wRegion == REGION_SCHWABEN;
    const bool wThreeQuarters = wRegion != REGION_RHEIN_RUHR && wRegion != REGION_WESSI;
    if (aMinute == 15)
    {
        AppendLayoutWord(wrWords, WORD_VIERTEL_1);
        if (wQuarterNext) wDisplay.mFlags = HOUR_OFFSET_1;
        else AppendLayoutWord(wrWords, WORD_NACH_1);
        return wDisplay;
    }
    if (aMinute == 30 || aMinute == 45)
    {
        wDisplay.mFlags = HOUR_OFFSET_1;
        AppendLayoutWord(wrWords, aMinute == 30 ? WORD_HALB : wThreeQuarters ? WORD_DREIVIERTEL : WORD_VIERTEL_1);
        if (aMinute == 45 && !wThreeQuarters) AppendLayoutWord(wrWords, WORD_VOR_1);
        return wDisplay;
    }
    const bool wWideHalf = wRegion == REGION_OSSI || wRegion == REGION_WESSI;
    const bool wAroundHalf = wWideHalf ? (aMinute >= 20 && aMinute <= 44) : (aMinute >= 21 && aMinute <= 39);
    const bool wNextHour = wAroundHalf || aMinute > 30;
    const bool wBefore = wAroundHalf ? aMinute < 30 : aMinute > 30;
    const uint8_t wNumber = wAroundHalf ? (aMinute < 30 ? 30 - aMinute : aMinute - 30) :
        (aMinute > 30 ? 60 - aMinute : aMinute);
    wDisplay.mFlags = wNextHour ? HOUR_OFFSET_1 : NO_FLAGS;
    const bool wUseUnit = arConfig.mMinuteUnits || (arConfig.mBareSingularUnit && wNumber == 1);
    AppendLayoutNumber(wrWords, wNumber, false, wUseUnit ? ONE_EINE : ONE_EINS);
    if (wUseUnit) AppendLayoutWord(wrWords, wNumber == 1 ? WORD_MINUTE_1 : WORD_MINUTEN_1);
    AppendLayoutWord(wrWords, wBefore ? WORD_VOR_1 : WORD_NACH_1);
    if (wAroundHalf) AppendLayoutWord(wrWords, WORD_HALB);
    return wDisplay;
}

/** @brief Precompute all 26 modes x 60 minutes; sampling is accessor policy. */
constexpr auto MakeMinuteTable24h2()
{
    std::array<std::array<tMinuteDisplay, MINUTE_COUNT>, WORDCLOCK_MODE_NUMBER> wTable = {};
    for (unsigned wMode = 0; wMode < WORDCLOCK_MODE_NUMBER; ++wMode)
    {
        for (uint8_t wMinute = 0; wMinute < MINUTE_COUNT; ++wMinute)
        {
            wTable[wMode][wMinute] = MakeMinuteDisplay24h2(mcModeConfigs24h2[wMode], wMinute);
        }
    }
    return wTable;
}
static constexpr auto mcWordMinutesTable = MakeMinuteTable24h2();
/* [remainder 0..4][word slot]. All visible entries are on row 17:
 * '+' column 0; digits columns 1..4; MINUTE(N) columns 6..11/12.
 * '#' at column 5 is reserved spacing, not a word or an illuminated LED.
 */
static constexpr tWord mcWordExtraMinutesTable[EXTRA_MINUTE_COUNT][MAX_EXTRA_MINUTE_WORDS] =
{
    { WORD_END_OF_WORDS },
    { WORD_PLUS, WORD_NUM_1, WORD_MINUTE },
    { WORD_PLUS, WORD_NUM_2, WORD_MINUTEN },
    { WORD_PLUS, WORD_NUM_3, WORD_MINUTEN },
    { WORD_PLUS, WORD_NUM_4, WORD_MINUTEN }
};

/* Accessor inputs: valid mode/resolution, civil hour 0..23, minute 0..59. */
/** @brief Keep the civil minute in exact mode; otherwise floor to a multiple of five. */
constexpr uint8_t GetLayoutBaseMinute(uint8_t aMinute, tMinuteResolution24h2 aResolution = mcDefaultMinuteResolution)
{
    return aResolution == EXACT_MINUTES ? aMinute : aMinute - aMinute % 5;
}
/** @brief Select a full-minute table entry after applying resolution policy.
 * aMinute is a civil minute, NOT a /5 index. Inputs are not bounds-checked.
 */
constexpr tMinuteDisplay GetLayoutMinuteDisplay(tWordClockMode aMode, uint8_t aMinute,
    tMinuteResolution24h2 aResolution = mcDefaultMinuteResolution)
{
    return mcWordMinutesTable[aMode][GetLayoutBaseMinute(aMinute, aResolution)];
}
/** @brief Apply the phrase's next-hour flag and normalize to civil index 0..23. */
constexpr uint8_t GetLayoutHourIndex(uint8_t aHour, const tMinuteDisplay& arMinute)
{
    return (aHour + ((arMinute.mFlags & HOUR_OFFSET_1) ? 1 : 0)) % HOURS_COUNT;
}
/** @brief Remainder index 0..4; exact mode returns empty row zero. */
constexpr uint8_t GetLayoutExtraMinuteIndex(uint8_t aMinute,
    tMinuteResolution24h2 aResolution = mcDefaultMinuteResolution)
{
    return aResolution == EXACT_MINUTES ? 0 : aMinute % 5;
}
/** @brief Copy the selected hour words and resolve full-hour EIN versus EINS.
 * Uses the same sampled minute as the main phrase. Offset-adjusted hours
 * determine both number and day period, wrapping correctly at midnight.
 */
constexpr auto GetLayoutHourWords(tWordClockMode aMode, uint8_t aHour, uint8_t aMinute,
    tMinuteResolution24h2 aResolution = mcDefaultMinuteResolution)
{
    const auto wMinute = GetLayoutMinuteDisplay(aMode, aMinute, aResolution);
    const auto wHour = GetLayoutHourIndex(aHour, wMinute);
    auto wWords = mcWordHoursTable[wMinute.mHourMode][wHour];
    // Resolve the source's conditional EIN/EINS token before rendering.
    if (wMinute.mHourMode == HOUR_MODE_LOWER_12 && wHour % 12 == 1 &&
        GetLayoutBaseMinute(aMinute, aResolution) == 0)
    {
        wWords[0] = WORD_EIN_4;
    }
    return wWords;
}

/* Resolved IDs only: no upstream conditional tokens, lengths with flags, or
 * hidden ES IST members in the main group. Word order in these lists is not
 * print order: rendering selects spans; the physical panel supplies ordering.
 */
/** @brief Word-only scene proposal; zero entries are unused, not visible words.
 * The caller still owns visibility, colors and brightness for all three groups.
 */
struct tLayoutPresentation24h2
{
    std::array<tWord, 2> mItIs;
    std::array<tWord, LAYOUT_MAX_TIME_WORDS> mTime;
    std::array<tWord, MAX_EXTRA_MINUTE_WORDS> mExtraMinutes;
};
/** @brief Compose ES IST, main time and optional remainder without rendering.
 * @param aMode Valid clock mode; does not encode exact/stepped resolution.
 * @param aHour Civil hour 0..23.
 * @param aMinute Civil minute 0..59.
 * @param aResolution FIVE_MINUTES adds a remainder; EXACT_MINUTES spells it out.
 * @return Fixed-capacity, zero-padded word groups, with no colors or side effects.
 */
constexpr auto GetLayoutPresentation(tWordClockMode aMode, uint8_t aHour, uint8_t aMinute,
    tMinuteResolution24h2 aResolution = mcDefaultMinuteResolution)
{
    tLayoutPresentation24h2 wScene = { { WORD_ES, WORD_IST }, {}, {} };
    const auto wMinute = GetLayoutMinuteDisplay(aMode, aMinute, aResolution);
    for (tWord wWord : wMinute.wMinuteWords) AppendLayoutWord(wScene.mTime, wWord);
    for (tWord wWord : GetLayoutHourWords(aMode, aHour, aMinute, aResolution)) AppendLayoutWord(wScene.mTime, wWord);
    const uint8_t wExtra = GetLayoutExtraMinuteIndex(aMinute, aResolution);
    for (unsigned wIndex = 0; wIndex < MAX_EXTRA_MINUTE_WORDS; ++wIndex)
    {
        wScene.mExtraMinutes[wIndex] = mcWordExtraMinutesTable[wExtra][wIndex];
    }
    return wScene;
}

/* Check spelling, bounds, preview encoding, and BOTH directions of coverage:
 * no word spans '#', and no non-'#' cell is left outside the vocabulary.
 */
constexpr bool ValidateLayout24h2()
{
    bool wCovered[MATRIX_HEIGHT][MATRIX_WIDTH] = {};
    for (unsigned wWord = 1; wWord < WORD_MAX_NUMBER; ++wWord)
    {
        const auto& wrData = mcWordDataArray[wWord];
        if (wrData.mRow >= MATRIX_HEIGHT || wrData.mLength == 0 || wrData.mColumn + wrData.mLength > MATRIX_WIDTH) return false;
        for (unsigned wOffset = 0; wOffset < wrData.mLength; ++wOffset)
        {
            const unsigned wColumn = wrData.mColumn + wOffset;
            const char wGlyph = mcLayoutCells[wrData.mRow][wColumn];
            if (wGlyph == '#' || wGlyph != mcWordSpellings[wWord][wOffset]) return false;
            wCovered[wrData.mRow][wColumn] = true;
        }
    }
    for (unsigned wRow = 0; wRow < MATRIX_HEIGHT; ++wRow)
    {
        for (unsigned wColumn = 0; wColumn < MATRIX_WIDTH; ++wColumn)
        {
            const char wGlyph = mcLayoutCells[wRow][wColumn];
            if (wGlyph == '\0' || (wGlyph != '#') != wCovered[wRow][wColumn] ||
                mDisplayLayout[wRow][wColumn * 3 + 2] != wGlyph) return false;
        }
        if (mcLayoutCells[wRow][MATRIX_WIDTH] != '\0' || mDisplayLayout[wRow][MATRIX_WIDTH * 3 + 2] != '\0') return false;
    }
    for (const auto& wrMode : mcWordMinutesTable)
    {
        for (const auto& wrMinute : wrMode)
        {
            if (wrMinute.mHourMode >= HOUR_MODE_MAX_NUMBER || wrMinute.mFlags > HOUR_OFFSET_1) return false;
            for (tWord wWord : wrMinute.wMinuteWords) if (wWord >= WORD_MAX_NUMBER) return false;
        }
    }
    for (const auto& wrMode : mcWordHoursTable)
    {
        for (const auto& wrHour : wrMode)
        {
            for (tWord wWord : wrHour) if (wWord >= WORD_MAX_NUMBER) return false;
        }
    }
    for (const auto& wrExtra : mcWordExtraMinutesTable)
    {
        for (tWord wWord : wrExtra)
        {
            if (wWord >= WORD_MAX_NUMBER || (wWord != 0 && mcWordDataArray[wWord].mRow != 17)) return false;
        }
    }
    return true;
}
static_assert(WORD_MAX_NUMBER < 256, "Word IDs and overflow marker must fit uint8_t");
static_assert(ValidateLayout24h2(), "Invalid 18x18 layout, spelling, reserved cell or table");
static_assert(GetLayoutHourIndex(12, GetLayoutMinuteDisplay(WORDCLOCK_MODE_24H, 0)) == 12, "Preserve noon");
static_assert(GetLayoutHourIndex(23, GetLayoutMinuteDisplay(WORDCLOCK_MODE_1, 55)) == 0, "Wrap midnight");
static_assert(GetLayoutHourWords(WORDCLOCK_MODE_0, 1, 1)[0] == WORD_EIN_4, "Stepped :01 uses EIN UHR +1 Minute");
static_assert(GetLayoutHourWords(WORDCLOCK_MODE_0, 1, 1, EXACT_MINUTES)[0] == WORD_EINS_2, "Exact :01 uses EINS after NACH");
static_assert(GetLayoutExtraMinuteIndex(59) == 4 && GetLayoutExtraMinuteIndex(59, EXACT_MINUTES) == 0, "Do not double-count exact minutes");
static_assert(mcWordMinutesTable[WORDCLOCK_MODE_24H][1].wMinuteWords[1] == WORD_EINE_2, "EINE MINUTE");
static_assert(mcWordMinutesTable[WORDCLOCK_MODE_24H][16].wMinuteWords[1] == WORD_SECH_2 &&
    mcWordMinutesTable[WORDCLOCK_MODE_24H][17].wMinuteWords[1] == WORD_SIEB_2, "SECHZEHN / SIEBZEHN");
static_assert(mcWordMinutesTable[WORDCLOCK_MODE_24H][59].wMinuteWords[4] == WORD_MINUTEN_2, "Longest numeric minute phrase fits");
static_assert(mcWordMinutesTable[WORDCLOCK_MODE_OSSI][15].mFlags == HOUR_OFFSET_1 &&
    mcWordMinutesTable[WORDCLOCK_MODE_RHEIN_RUHR][15].mFlags == NO_FLAGS, "Regional quarter-hour distinction");
static_assert(mcWordMinutesTable[WORDCLOCK_MODE_OSSI][1].wMinuteWords[0] == WORD_EINE_1 &&
    mcWordMinutesTable[WORDCLOCK_MODE_OSSI][1].wMinuteWords[1] == WORD_MINUTE_1 &&
    mcWordMinutesTable[WORDCLOCK_MODE_TIROL][29].wMinuteWords[1] == WORD_MINUTE_1 &&
    mcWordMinutesTable[WORDCLOCK_MODE_RHEIN_RUHR][59].wMinuteWords[1] == WORD_MINUTE_1,
    "Source-derived bare regional modes retain EINE MINUTE");