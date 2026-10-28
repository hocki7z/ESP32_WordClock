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
static constexpr tColor mIntroColor = 0xFFA500;     // CRGB::Orange


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
    PaintWord(WORD_WORDCLOCK, mIntroColor & COLOR_RGB_MASK);

    /* Show new data on the LED matrix */
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
    /* LOG */
    LOG(LOG_DEBUG, "Display.UpdateDisplay() Update display for time %02u:%02u",
            mDateTime.mTime.mHour,  mDateTime.mTime.mMinute);

    /* Prepare time scene */
    bool wUpdated = DisplayTimeScene();

    /* Display scene changed */
    if (wUpdated)
    {
        /* Show new data on the LED matrix */
        FastLED.show();

        /* Publish a compact snapshot for the web UI */
        MessageNS::Message wMessage{};
        wMessage.mSource = MessageNS::tAddress::DISPLAY_MANAGER;
        wMessage.mDestination = MessageNS::tAddress::WEB_MANAGER;
        wMessage.mId = MessageNS::tMessageId::MSG_EVENT_DISPLAY_FRAME_READY;

        SendMessage(wMessage);
    }
}

/**
 * @brief Builds and displays the scene for the current time.
 *
 * @details Reads display settings, creates the word groups for the current
 *          time, optionally logs the scene, then renders each enabled group
 *          using its configured color.
 */
bool Display::DisplayTimeScene(void)
{
    bool wUpdated = false;

    uint32_t wDwordValue;

    tColor  wColorTime;
    tColor  wColorBkgd;
    uint8_t wBrightnessTime;
    uint8_t wBrightnessBkgd;

    bool wIsDayMode = IsDayMode();

    /* Retrieve values from settings */
    if (wIsDayMode)
    {
        wDwordValue = Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayColorTime, ConfigNS::mDefaultDisplayColorTime);
        wColorTime  = static_cast<tColor>(wDwordValue & COLOR_RGB_MASK);

        wDwordValue = Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayColorBkgd, ConfigNS::mDefaultDisplayColorBkgd);
        wColorBkgd  = static_cast<tColor>(wDwordValue & COLOR_RGB_MASK);

        wBrightnessTime = Settings.GetValue<uint8_t>(
            ConfigNS::mKeyDisplayBrightnessTime, ConfigNS::mDefaultDisplayBrightnessTime);
        wBrightnessBkgd = Settings.GetValue<uint8_t>(
            ConfigNS::mKeyDisplayBrightnessBkgd, ConfigNS::mDefaultDisplayBrightnessBkgd);
    }
    else
    {
        wDwordValue = Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayColorTimeNight, ConfigNS::mDefaultDisplayColorTimeNight);
        wColorTime  = static_cast<tColor>(wDwordValue & COLOR_RGB_MASK);

        wDwordValue = Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayColorBkgdNight, ConfigNS::mDefaultDisplayColorBkgdNight);
        wColorBkgd  = static_cast<tColor>(wDwordValue & COLOR_RGB_MASK);

        wBrightnessTime = Settings.GetValue<uint8_t>(
            ConfigNS::mKeyDisplayBrightnessTimeNight, ConfigNS::mDefaultDisplayBrightnessTime);
        wBrightnessBkgd = Settings.GetValue<uint8_t>(
            ConfigNS::mKeyDisplayBrightnessBkgdNight, ConfigNS::mDefaultDisplayBrightnessBkgdNight);
    }

//    LOG(LOG_DEBUG, "Display::DisplayTimeScene() IsDayMode %d; Display colors: Time=0x%06X, Bkgd=0x%06X, Brightness: Time=%u%%, Bkgd=%u%%",
//        wIsDayMode, wColorTime, wColorBkgd, wBrightnessTime, wBrightnessBkgd);

    /* Prepare display scene */
    mDisplayScene.Clear();
    mDisplayScene.mColorBkgd      = wColorBkgd;
    mDisplayScene.mBrightnessBkgd = wBrightnessBkgd;

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
        mDisplayScene.mItIs.mColor      = wColorTime;
        mDisplayScene.mItIs.mBrightness = wBrightnessTime;

        mDisplayScene.mTime.SetEnabled(true);       // Always display time words
        mDisplayScene.mTime.mColor      = wColorTime;
        mDisplayScene.mTime.mBrightness = wBrightnessTime;

        mDisplayScene.mExtraMinutes.SetEnabled(wExtraMinutesEnabled);
        mDisplayScene.mExtraMinutes.mColor      = wColorTime;
        mDisplayScene.mExtraMinutes.mBrightness = wBrightnessTime;

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

    /* Store the display scene snapshot for the web UI */
    wUpdated = mDisplaySceneSnapshot.Store(mDisplayScene);

    if (wUpdated)
    {
#if (LOG_LEVEL == LOG_VERBOSE)      // don't compile this code each time
        BitMatrix wBitMatrix(MATRIX_WIDTH, MATRIX_HEIGHT);

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

        /* Log the display scene */
        LOG(LOG_VERBOSE, "Display::DisplayTimeScene() Display scene changed:");
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

        /* Lambda function to display a word group */
        auto wDisplayWordGroup = [this](const WordGroup& arGroup)
        {
            if (arGroup.IsEnabled())
            {
                BitMatrix wBitMatrix(MATRIX_WIDTH, MATRIX_HEIGHT);

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
                tColor wColor = ScaleColor(arGroup.mColor, arGroup.mBrightness);
                SetLedColor(wBitMatrix, CRGB(wColor & COLOR_RGB_MASK));
            }
        };

        /* Fill background */
        wColorBkgd = ScaleColor(mDisplayScene.mColorBkgd, mDisplayScene.mBrightnessBkgd);
        Fill(CRGB(wColorBkgd & COLOR_RGB_MASK));

        /* Display word groups */
        wDisplayWordGroup(mDisplayScene.mItIs);
        wDisplayWordGroup(mDisplayScene.mTime);
        wDisplayWordGroup(mDisplayScene.mExtraMinutes);
    }
    else
    {
        LOG(LOG_VERBOSE, "Display::DisplayTimeScene() Display scene unchanged");
    }

    return wUpdated;
}

bool Display::IsDayMode(void) const
{
    bool wIsDayMode = true;

    /* Check if day/night mode is enabled */
    bool wUseDayNightMode = Settings.GetValue<bool>(
            ConfigNS::mKeyDisplayUseDayNightMode, ConfigNS::mDefaultDisplayUseDayNightMode);

    if (wUseDayNightMode)
    {
        uint32_t wDwordValue = 0;

        /* Retrieve day mode start and end times */
        wDwordValue = Settings.GetValue<uint32_t>(
                ConfigNS::mKeyDisplayDayModeStartTime, ConfigNS::mDefaultDisplayDayModeStartTime);
        DateTimeNS::tDateTime wDayModeStartDateTime = DateTimeNS::DwordToDateTime(wDwordValue);

        wDwordValue = Settings.GetValue<uint32_t>(
                ConfigNS::mKeyDisplayDayModeEndTime, ConfigNS::mDefaultDisplayDayModeEndTime);
        DateTimeNS::tDateTime wDayModeEndDateTime = DateTimeNS::DwordToDateTime(wDwordValue);

        /* Determine if we are in the day mode */
        wIsDayMode = DateTimeNS::IsTimeInInterval(
            mDateTime.mTime,
            wDayModeStartDateTime.mTime,
            wDayModeEndDateTime.mTime);

        LOG(LOG_VERBOSE, "Display::IsDayMode() Day/Night mode enabled; Day mode start: %02u:%02u, end: %02u:%02u; Current time: %02u:%02u; IsDayMode: %d",
            wDayModeStartDateTime.mTime.mHour, wDayModeStartDateTime.mTime.mMinute,
            wDayModeEndDateTime.mTime.mHour,   wDayModeEndDateTime.mTime.mMinute,
            mDateTime.mTime.mHour, mDateTime.mTime.mMinute,
            wIsDayMode);
    }
    else
    {
        LOG(LOG_VERBOSE, "Display::IsDayMode() Day/Night mode disabled; Always day mode");
    }

    return wIsDayMode;
}

tColor Display::ScaleColor(const tColor aColor, const uint8_t aBrightness) const
{
    uint8_t wBrightness = (aBrightness > 100) ? 100 : aBrightness;
    uint8_t wAlphaScale = map(wBrightness, 0, 100, 0, 255);

    CRGB wLedColor = CRGB(aColor & COLOR_RGB_MASK);
    wLedColor.nscale8(wAlphaScale);

    return static_cast<tColor>(wLedColor.as_uint32_t() & COLOR_RGB_MASK);
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
