/*
 * Display.h
 *
 *  Created on: 02.09.2025
 *      Author: hocki
 */

#include <Arduino.h>

#include "Logger.h"
#include "Configuration.h"
#include "Serialize.h"
#include "Settings.hpp"

#include "Display.h"


/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)


/* Delay in msec between display updates */
static constexpr uint32_t mcUpdateDelay = 10;

/* Intro color */
static constexpr CRGB mIntroColor = CRGB::Orange;


/**
 * @brief Constructor
 */
Display::Display(char const* apName, ApplicationNS::tTaskPriority aPriority, const uint32_t aStackSize)
    : ApplicationNS::Task(apName, aPriority, aStackSize)
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

void Display::Init(ApplicationNS::tTaskObjects* apTaskObjects)
{
    /* Initialize base class */
    ApplicationNS::Task::Init(apTaskObjects);

    /* Initialize FastLED */
    // Initialize LEDs
    FastLED.addLeds<LED_TYPE, LED_DATA_PIN, LED_COLOR_ORDER>(mLeds, LED_NUMBER);
    // Disable dithering mode
	FastLED.setDither(DISABLE_DITHER);
    // Color-corrected LED brightness
    FastLED.setCorrection(TypicalLEDStrip);
    // Set default LED brightness
    FastLED.setBrightness(LED_DEFAULT_BRIGHTNESS);
    // Clear FastLED
    FastLED.clear();

    /* Switch OFF all LEDs */
    Clear();

    /* Display intro */
    PaintWord(WORD_WORDCLOCK, mIntroColor);

    FastLED.show();
}

void Display::ProcessIncomingMessage(const MessageNS::Message &arMessage)
{
    LOG(LOG_VERBOSE, "Display::ProcessIncomingMessage()");

    switch (arMessage.mId)
    {
        case MessageNS::tMessageId::MGS_EVENT_DATETIME_CHANGED:
        {
            /* Deserialize received time */
            uint32_t wDword;
            if (SerializeNS::DeserializeData(arMessage.mPayload, &wDword) == sizeof(wDword))
            {
                /* Store new date and time */
                mDateTime = DateTimeNS::DwordToDateTime(wDword);

                LOG(LOG_DEBUG, "Display::ProcessIncomingMessage() Datetime changed: " PRINTF_DATETIME_PATTERN,
                        PRINTF_DATETIME_FORMAT(mDateTime));

                /* Update display */
                UpdateDisplay();
            }
        }
            break;

        case MessageNS::tMessageId::MSG_EVENT_SETTINGS_CHANGED:
        {
            // Settings changed, re-read settings if needed
            LOG(LOG_DEBUG, "Display::ProcessIncomingMessage() Settings changed");

            /* Update display */
            UpdateDisplay();
        }
            break;

        default:
            // do nothing
            break;
    }
}

void Display::Clear(void)
{
    Fill(CRGB::Black);
}

void Display::Fill(const CRGB aColor, const uint8_t aBrightness)
{
    uint8_t wBrightness = (aBrightness > 100) ? 100 : aBrightness;
    uint8_t wAlphaScale = map(wBrightness, 0, 100, 0, 255);

    fill_solid(mLeds, LED_NUMBER, aColor.scale8(wAlphaScale));
}

void Display::UpdateDisplay(void)
{
    uint32_t wDwordValue = 0;

    /* LOG */
    LOG(LOG_DEBUG, "Display.UpdateDisplay() Update display for time %02u:%02u",
            mDateTime.mTime.mHour,  mDateTime.mTime.mMinute);

    /**
     * Update display data
     */

    /* Retrieve background color from settings */
    wDwordValue = Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayColorBkgd, ConfigNS::mDefaultDisplayColorBkgd);
    /*     and fill background */
    Fill(CRGB(wDwordValue & 0x00FFFFFF));

    /* Retrieve time color from settings */
    wDwordValue = Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayColorTime, ConfigNS::mDefaultDisplayColorTime);
    /*     paint time */
    PaintTime(mDateTime.mTime.mHour, mDateTime.mTime.mMinute, CRGB(wDwordValue & 0x00FFFFFF));


    /* Retrieve LED brightness */
    uint8_t wBrightness = Settings.GetValue<uint8_t>(
            ConfigNS::mKeyDisplayLedBrightness, ConfigNS::mDefaultDisplayLedBrightness);

    /* Check if night mode is enabled */
    bool wUseNightMode = Settings.GetValue<bool>(
            ConfigNS::mKeyDisplayUseNightMode, ConfigNS::mDefaultDisplayUseNightMode);

    if (wUseNightMode)
    {
        /* Retrieve night mode start and end times */
        wDwordValue = Settings.GetValue<uint32_t>(
                ConfigNS::mKeyDisplayNightModeStartTime, ConfigNS::mDefaultDisplayNightModeStartTime);
        DateTimeNS::tDateTime wNightModeStartDateTime = DateTimeNS::DwordToDateTime(wDwordValue);

        wDwordValue = Settings.GetValue<uint32_t>(
                ConfigNS::mKeyDisplayNightModeEndTime, ConfigNS::mDefaultDisplayNightModeEndTime);
        DateTimeNS::tDateTime wNightModeEndDateTime = DateTimeNS::DwordToDateTime(wDwordValue);

        /* Determine if we are in the night mode */
        bool wIsNightMode = DateTimeNS::IsTimeInInterval(
                mDateTime.mTime,
                wNightModeStartDateTime.mTime,
                wNightModeEndDateTime.mTime);

        if (wIsNightMode)
        {
            /* Apply night mode brightness */
            wBrightness = Settings.GetValue<uint8_t>(
                    ConfigNS::mKeyDisplayBrightnessNightMode, ConfigNS::mDefaultDisplayBrightnessNightMode);
        }
    }

    /* Set the brightness value to a range of 0–100% */
    if (wBrightness > 100)
    {
        wBrightness = 100;
    }

    /* Convert the brightness value (0–100%) to a range of the LED hardware scale (0–255) */
    uint8_t wAlphaScale = map(wBrightness, 0, 100, 0, 255);

    /* Set LED brightness */
    FastLED.setBrightness(wAlphaScale);

    /* Show new data on the LED matrix */
    FastLED.show();
}

void Display::SetLedColor(const uint16_t aLedIndex, const CRGB aColor)
{
    if (aLedIndex < LED_NUMBER)
    {
        mLeds[aLedIndex] = aColor;
    }
}

void Display::SetLedColor(BitMatrix& arLedMask, const CRGB aColor)
{
    if (arLedMask.GetSize() == LED_NUMBER)
    {
        for (uint16_t wI = 0; wI < arLedMask.GetSize(); wI++)
        {
            if (arLedMask.IsBitSet(wI))
            {
                mLeds[wI] = aColor;
            }
        }
    }
}

void Display::PaintWord(const tWord aWord, const CRGB aColor)
{
    /* Prepare LED mask */
    mLedMask.ClearAll();

    /* Check input parameters */
    if ((aWord > WORD_END_OF_WORDS) &&
        (aWord < WORD_MAX_NUMBER))
    {
        /* Get word data */
        tWordData wWordData = mcWordDataArray[aWord];
        /* Set LED bits for the word */
        mLedMask.SetLine(wWordData.mRow, wWordData.mColumn, wWordData.mLength);
    }

    /* Match the logical LED mask to the physical layout (zigzag order) */
    for (uint8_t wRow = 0; wRow < mLedMask.GetHeight(); wRow++)
    {
        if ((wRow % 2) == 0)
        {
            /* It is even row -> flip it */
            mLedMask.FlipRow(wRow);
        }
    }

    /* Now paint all LEDs marked in the mask */
    SetLedColor(mLedMask, aColor);
}

void Display::PaintTime(const uint8_t aHour, const uint8_t aMinute, const CRGB aColor)
{
    /* Prepare LED mask */
    mLedMask.ClearAll();

    /* Check input parameters */
    if ((aHour   < 24) &&       // Support only 24 hours format
        (aMinute < 60))         // 0..59 minutes
    {
        uint8_t wHour         = aHour;
        uint8_t wMinute       = aMinute / 5;    // minute steps 0, 5, ... 55
        uint8_t wMinuteExtra  = aMinute % 5;    // extra minutes 0, +1 ... +4

        tWordClockMode wMode  = WORDCLOCK_MODE_1;

        /* Get minute display data */
        tMinuteDisplay wMinuteDisplay = mcWordMinutesTable[wMode][wMinute];

        /* Correct hour offset */
        if ((wMinuteDisplay.mFlags & HOUR_OFFSET_1) == HOUR_OFFSET_1)
        {
            wHour += 1;
        }

        /* We have only 12 hours */
        while (wHour > HOURS_COUNT)
        {
            wHour -= HOURS_COUNT;
        }

        /* Correct index for 12 Hours */
        if (wHour == 12)
        {
            wHour = 0;
        }

        /* Prepare array of words to display the time */
        tWord wDisplayWords[MAX_MINUTE_WORDS + MAX_HOUR_WORDS + MAX_EXTRA_MINUTE_WORDS + 2] = { WORD_END_OF_WORDS }; // +2 for "ES" and "IST" words
        uint8_t wWordsOffset = 0;

        /* Add "ES IST" words */
        wDisplayWords[wWordsOffset++] = WORD_ES;
        wDisplayWords[wWordsOffset++] = WORD_IST;
        /* Add minute words */
        memccpy(&wDisplayWords[wWordsOffset], wMinuteDisplay.wMinuteWords,
            MAX_MINUTE_WORDS, sizeof(wMinuteDisplay.wMinuteWords));
        wWordsOffset += MAX_MINUTE_WORDS;
        /* Add hour words */
        memccpy(&wDisplayWords[wWordsOffset], mcWordHoursTable[wMinuteDisplay.mHourMode][wHour],
            MAX_HOUR_WORDS, sizeof(mcWordHoursTable[wMinuteDisplay.mHourMode][wHour]));
        wWordsOffset += MAX_HOUR_WORDS;
        /* Add extra minute words */
        memccpy(&wDisplayWords[wWordsOffset], mcWordExtraMinutesTable[wMinuteExtra],
            MAX_EXTRA_MINUTE_WORDS, sizeof(mcWordExtraMinutesTable[wMinuteExtra]));
        wWordsOffset += MAX_EXTRA_MINUTE_WORDS;

        /* Set LED bits for all display words */
        for (uint8_t wI = 0; wI < wWordsOffset; wI++)
        {
            if ((wDisplayWords[wI] > WORD_END_OF_WORDS) &&
                (wDisplayWords[wI] < WORD_MAX_NUMBER))
            {
                /* Get word data */
                tWordData wWordData = mcWordDataArray[wDisplayWords[wI]];
                /* Set LED bits for the word */
                mLedMask.SetLine(wWordData.mRow, wWordData.mColumn, wWordData.mLength);
            }
        }
    }

#if (LOG_LEVEL == LOG_VERBOSE)  // don't compile this code each time
    /* Log the LED layout with the painted time */
    LOG(LOG_VERBOSE, "Display::PaintTime() LED layout:");
    for (uint16_t wRow = 0; wRow < mLedMask.GetHeight(); wRow++)
    {
        String wLogRow;

        for (uint16_t wCol = 0; wCol < mLedMask.GetWidth(); wCol++)
        {
            if (mLedMask.IsBitSet(wRow, wCol))
            {
                wLogRow += mDisplayLayout[wRow][wCol*3 + 2];  // 2, 5, 8, ...
            }
            else
            {
                wLogRow += '.';
            }

            wLogRow += ' ';
        }
        /* Add row to the log */
        LOG(LOG_VERBOSE, "    %s", wLogRow.c_str());
    }
#endif /* (LOG_LEVEL == LOG_VERBOSE) */

    /* Match the logical LED mask to the physical layout (zigzag order) */
    for (uint8_t wRow = 0; wRow < mLedMask.GetHeight(); wRow++)
    {
        if ((wRow % 2) == 0)
        {
            /* It is even row -> flip it */
            mLedMask.FlipRow(wRow);
        }
    }

    /* Now paint all LEDs marked in the mask */
    SetLedColor(mLedMask, aColor);
}