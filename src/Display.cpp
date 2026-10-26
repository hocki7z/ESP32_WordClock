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

    /* Prepare display scene */
    mDisplayScene.Clear();
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

    
    DisplayTimeScene();

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

    /* Publish a compact snapshot for the web UI */
    MessageNS::Message wMessage{};
    wMessage.mSource = MessageNS::tAddress::DISPLAY_MANAGER;
    wMessage.mDestination = MessageNS::tAddress::WEB_MANAGER;
    wMessage.mId = MessageNS::tMessageId::MSG_EVENT_DISPLAY_FRAME_READY;

    SendMessage(wMessage);
}

/**
 * @brief Builds and displays the scene for the current time.
 *
 * @details Reads display settings, creates the word groups for the current
 *          time, optionally logs the scene, then renders each enabled group
 *          using its configured color.
 */
void Display::DisplayTimeScene(void)
{
    BitMatrix wBitMatrix(MATRIX_WIDTH, MATRIX_HEIGHT);

    /* Retrieve background color from settings */
    uint32_t wDwordValue = Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayColorBkgd, ConfigNS::mDefaultDisplayColorBkgd);
    tColor wBackgroundColor = static_cast<tColor>(wDwordValue & COLOR_RGB_MASK);

    /* Retrieve time color from settings */
    wDwordValue = Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayColorTime, ConfigNS::mDefaultDisplayColorTime);
    tColor   wTimeColor  = static_cast<tColor>(wDwordValue & COLOR_RGB_MASK);

    /* Prepare display scene */
    mDisplayScene.Clear();
    mDisplayScene.mBackground = wBackgroundColor;


    /* Check current time validity */
    if ((mDateTime.mTime.mHour   < 24) &&       // Support only 24 hours format
        (mDateTime.mTime.mMinute < 60))         //     and 0..59 minutes
    {
        uint8_t wHour         = mDateTime.mTime.mHour;
        uint8_t wMinute       = mDateTime.mTime.mMinute / 5;    // Minute  steps 0, 5, ... 55
        uint8_t wMinuteExtra  = mDateTime.mTime.mMinute % 5;    // Extra minutes 0, +1 ... +4

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

        /* Check if "IT IS" words should be displayed */
        bool  wItIsEnabled = Settings.GetValue<bool>(ConfigNS::mKeyDisplayClockItIs, ConfigNS::mDefaultDisplayClockItIs);
        /* Check if extra minutes should be displayed */
        bool  wExtraMinutesEnabled = Settings.GetValue<bool>(ConfigNS::mKeyDisplayClockSingleMins, ConfigNS::mDefaultDisplayClockSingleMins);

        mDisplayScene.mItIs.SetEnabled(wItIsEnabled);
        mDisplayScene.mItIs.mColor   = wTimeColor;

        mDisplayScene.mTime.SetEnabled(true);       // Always display time words
        mDisplayScene.mTime.mColor = wTimeColor;

        mDisplayScene.mExtraMinutes.SetEnabled(wExtraMinutesEnabled);
        mDisplayScene.mExtraMinutes.mColor = wTimeColor;

        /* Add "IT IS" words */
        mDisplayScene.mItIs.mWords[0] = WORD_ES;
        mDisplayScene.mItIs.mWords[1] = WORD_IST;
        mDisplayScene.mItIs.mWordsCount = 2;

        /* Add minute words to displayed time */
        memccpy(&mDisplayScene.mTime.mWords[0], wMinuteDisplay.wMinuteWords,
            MAX_MINUTE_WORDS, sizeof(wMinuteDisplay.wMinuteWords));

        /* Add hour words to displayed time */
        memccpy(&mDisplayScene.mTime.mWords[MAX_MINUTE_WORDS], mcWordHoursTable[wMinuteDisplay.mHourMode][wHour],
            MAX_HOUR_WORDS, sizeof(mcWordHoursTable[wMinuteDisplay.mHourMode][wHour]));

        mDisplayScene.mTime.mWordsCount = MAX_MINUTE_WORDS + MAX_HOUR_WORDS;

        /* Add extra minute words */
        memccpy(&mDisplayScene.mExtraMinutes.mWords[0], mcWordExtraMinutesTable[wMinuteExtra],
            MAX_EXTRA_MINUTE_WORDS, sizeof(mcWordExtraMinutesTable[wMinuteExtra]));

        mDisplayScene.mExtraMinutes.mWordsCount = MAX_EXTRA_MINUTE_WORDS;
    }

#if (LOG_LEVEL == LOG_VERBOSE)      // don't compile this code each time
    wBitMatrix.ClearAll();

    auto wAddGroupToBitMatrix = [&wBitMatrix](const WordGroup& arGroup)
    {
        if (arGroup.IsEnabled())
        {
            for (uint8_t wI = 0; wI < arGroup.mWordsCount; wI++)
            {
                const tWord wWord = arGroup.mWords[wI];

                if ((wWord > WORD_END_OF_WORDS) &&
                    (wWord < WORD_MAX_NUMBER))
                {
                    const tWordData& wData = mcWordDataArray[wWord];
                    wBitMatrix.SetLine(wData.mRow, wData.mColumn, wData.mLength);
                }
            }
        }
    };

    wAddGroupToBitMatrix(mDisplayScene.mItIs);
    wAddGroupToBitMatrix(mDisplayScene.mTime);
    wAddGroupToBitMatrix(mDisplayScene.mExtraMinutes);

    /* Log the display scene with the painted time */
    LOG(LOG_VERBOSE, "Display::PaintTime() Display scene:");
    for (uint16_t wRow = 0; wRow < wBitMatrix.GetHeight(); wRow++)
    {
        String wLogRow;

        for (uint16_t wCol = 0; wCol < wBitMatrix.GetWidth(); wCol++)
        {
            if (wBitMatrix.IsBitSet(wRow, wCol))
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

    mDisplaySceneSnapshot.Store(mDisplayScene);

    auto wDisplayWordGroup = [this, &wBitMatrix](const WordGroup& arGroup)
    {
        if (arGroup.IsEnabled())
        {
            wBitMatrix.ClearAll();

            for (uint8_t wI = 0; wI < arGroup.mWordsCount; wI++)
            {
                const tWord wWord = arGroup.mWords[wI];

                if ((wWord > WORD_END_OF_WORDS) &&
                    (wWord < WORD_MAX_NUMBER))
                {
                    const tWordData& wData = mcWordDataArray[wWord];
                    wBitMatrix.SetLine(wData.mRow, wData.mColumn, wData.mLength);
                }
            }

            /* Match the logical LED mask to the physical layout (zigzag order) */
            wBitMatrix.SetZigzagOrder(true);

            /* Now paint all LEDs marked in the mask */
            SetLedColor(wBitMatrix, CRGB(arGroup.mColor & COLOR_RGB_MASK));
        }
    };

    /* Fill background */
    Fill(CRGB(mDisplayScene.mBackground & COLOR_RGB_MASK));

    /* Display word groups */
    wDisplayWordGroup(mDisplayScene.mItIs);
    wDisplayWordGroup(mDisplayScene.mTime);
    wDisplayWordGroup(mDisplayScene.mExtraMinutes);
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
    BitMatrix wWordMask(MATRIX_WIDTH, MATRIX_HEIGHT);

    /* Prepare LED mask */
    wWordMask.ClearAll();

    /* Check input parameters */
    if ((aWord > WORD_END_OF_WORDS) &&
        (aWord < WORD_MAX_NUMBER))
    {
        /* Get word data */
        tWordData wWordData = mcWordDataArray[aWord];
        /* Set LED bits for the word */
        wWordMask.SetLine(wWordData.mRow, wWordData.mColumn, wWordData.mLength);
    }

    /* Match the logical LED mask to the physical layout (zigzag order) */
    wWordMask.SetZigzagOrder(true);

    /* Now paint all LEDs marked in the mask */
    SetLedColor(wWordMask, aColor);
}
