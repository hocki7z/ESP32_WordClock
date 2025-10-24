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
constexpr uint32_t mcUpdateDelay = 10;

static constexpr uint8_t mBackgroundBrightness = 2;

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
    /* LOG */
    LOG(LOG_DEBUG, "Display.UpdateDisplay() Update display for time %02u:%02u",
            mDateTime.mTime.mHour,  mDateTime.mTime.mMinute);

    /**
     * Update display data
     */
    /* Retrieve background color from settings */
    uint32_t wColorCode = Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayColorBkgd, ConfigNS::mDefaultDisplayColorBkgd);
    /*     and fill background */
    Fill(CRGB(wColorCode & 0x00FFFFFF), mBackgroundBrightness);
    /* Retrieve time color from settings */
    wColorCode = Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayColorTime, ConfigNS::mDefaultDisplayColorTime);
    /*     paint time */
    PaintTime(mDateTime.mTime.mHour, mDateTime.mTime.mMinute, CRGB(wColorCode & 0x00FFFFFF));

    /* Transform data for LED matrix */
    Transform();

    /* Show new data on the LED matrix */
    FastLED.show();
}

void Display::Transform(void)
{
    for (uint8_t wRow = 0; wRow < MATRIX_HEIGHT; wRow++)
    {
        bool wReverseRow = false;

        if ((wRow % 2) == 0)
        {
            // it is even row
            wReverseRow = true;
        }
        else
        {
            // it is odd row
        }

        if (wReverseRow)
        {
            uint16_t wRowStart = (wRow * MATRIX_WIDTH); //  e.g. 0 / 16 / 32 / 48 ... 224 / 240

            /* Pointer to first element */
            CRGB* wLeftElem  = &mLeds[wRowStart];
            /* Pointer to last element */
            CRGB* wRightElem = &mLeds[wRowStart + MATRIX_WIDTH - 1] ;

            while (wLeftElem < wRightElem)
            {
                CRGB wTemp  = *wLeftElem;
                *wLeftElem  = *wRightElem;
                *wRightElem = wTemp;

                wLeftElem++;
                wRightElem--;
            }
        }
    }
}

void Display::SetColor(const uint16_t aLedIndex, const CRGB aColor)
{
    if (aLedIndex < LED_NUMBER)
    {
        mLeds[aLedIndex] = aColor;
    }
}

void Display::PaintPixel(const uint16_t aRow, const uint16_t aCol, const CRGB aColor)
{
    /* Check input parameters */
    if ((aRow < MATRIX_HEIGHT) &&
        (aCol < MATRIX_WIDTH))
    {
        /* Calculate LED index */
        uint16_t wLedIndex = (aRow * MATRIX_HEIGHT) + aCol;

        /* Set LED color */
        SetColor(wLedIndex, aColor);
    }
}

void Display::PaintLine(const uint16_t aRow, const uint16_t aCol, const uint16_t aLength, const CRGB aColor)
{
    /* Check input parameters */
    if ((aRow < MATRIX_HEIGHT) &&
        (aCol < MATRIX_WIDTH)  && ((aCol + aLength) <= MATRIX_WIDTH))
    {
        for (uint16_t wI = 0; wI < aLength; wI++)
        {
            PaintPixel(aRow, (aCol + wI), aColor);
        }
    }
}

void Display::PaintArea(const uint16_t aRow, const uint16_t aCol, const uint16_t aWidth, const uint16_t aHeight, const CRGB aColor)
{
    /* Check input parameters */
    if ((aRow < MATRIX_HEIGHT) && ((aRow + aHeight) <= MATRIX_HEIGHT) &&
        (aCol < MATRIX_WIDTH)  && ((aCol + aWidth)  <= MATRIX_WIDTH))
    {
        for (uint16_t wI = 0; wI < aHeight; wI++)
        {
            PaintLine((aRow + wI), aCol, aWidth, aColor);
        }
    }
}

void Display::PaintWord(const tWord aWord, const CRGB aColor)
{
    /* Check input parameters */
    if ((aWord > WORD_END_OF_WORDS) &&
        (aWord < WORD_MAX_NUMBER))
    {
        /* Get word data */
        tWordData wWordData = mcWordDataArray[aWord];

        if (wWordData.mLength > 0)
        {
            /* Paint word as a line */
            PaintLine(wWordData.mRow, wWordData.mColumn, wWordData.mLength, aColor);
        }
    }
}

void Display::PaintTime(const uint8_t aHour, const uint8_t aMinute, const CRGB aColor)
{
    uint8_t wI;

    /* Check input parameters */
    if ((aHour < 24) &&         // we support only 24 hours format
        (aMinute < 60))         //                 0..59 minutes
    {
        uint8_t wHour         = aHour;
        uint8_t wMinute       = aMinute / 5;    // minute steps 0, 5, ... 55
        uint8_t wMinuteExtra  = aMinute % 5;    // extra minutes 0, +1 ... +4

        tWordClockMode wClockMode = static_cast<tWordClockMode>(
                Settings.GetValue<uint8_t>(ConfigNS::mKeyDisplayClockMode,
                        ConfigNS::mDefaultDisplayClockMode));

        /* Get minute display data */
        tMinuteDisplay wMinuteDisplay = mcWordMinutesTable[wClockMode][wMinute];

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

        if (Settings.GetValue<bool>(
                ConfigNS::mKeyDisplayClockItIs, ConfigNS::mDefaultDisplayClockItIs))
        {
            /* Paint 'Es ist' */
            PaintWord(tWord::WORD_ES,  aColor);
            PaintWord(tWord::WORD_IST, aColor);
        }

        /* Paint minutes */
        for (wI = 0; wI < MAX_MINUTE_WORDS; wI++)
        {
            PaintWord(wMinuteDisplay.wMinuteWords[wI],  aColor);
        }

        /* Paint hours */
        for (wI = 0; wI < MAX_HOUR_WORDS; wI++)
        {
            PaintWord(mcWordHoursTable[wMinuteDisplay.mHourMode][wHour][wI], aColor);
        }

        if (Settings.GetValue<bool>(
                ConfigNS::mKeyDisplayClockSingleMins, ConfigNS::mDefaultDisplayClockSingleMins))
        {
            /* Paint extra minutes */
            for (wI = 0; wI < MAX_EXTRA_MINUTE_WORDS; wI++)
            {
                PaintWord(mcWordExtraMinutesTable[wMinuteExtra][wI],  aColor);
            }
        }
    }
}