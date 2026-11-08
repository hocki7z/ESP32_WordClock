/*
 * Display.h
 *
 *  Created on: 02.09.2025
 *      Author: hocki
 */
#pragma once

#include <FastLED.h>

#include "Application.h"

#include "DisplayLayout.h"
#include "DateTime.h"
#include "BitMatrix.h"


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


class Display : public ApplicationNS::Task
{
public:
    Display(char const* apName, ApplicationNS::tTaskPriority aPriority, const uint32_t aStackSize);
    virtual ~Display();

    /* ApplicationNS::Task::Init */
    void Init(ApplicationNS::tTaskObjects* apTaskObjects) override;

    /* ApplicationNS::Task::task */
    void task(void) override;

private:
    /** @brief Display states */
    enum tDisplayState : uint8_t
    {
        STATE_INTRO,            // Intro is shown, waiting for intro delay and startup text
        STATE_SCROLL_TEXT,      // Text is scrolling
        STATE_TIME,             // Time is displayed
    };

    /* Timer IDs */
    static constexpr uint32_t mIntroTimerId  = 0x01;
    static constexpr uint32_t mScrollTimerId = 0x02;

    /* Timers */
    ApplicationNS::tTaskTimerObjects mTimerObjects;
    ApplicationNS::TaskTimer* mpIntroTimer  = nullptr;
    ApplicationNS::TaskTimer* mpScrollTimer = nullptr;

    /* Current display state */
    tDisplayState mState = STATE_INTRO;
    /* Intro delay elapsed */
    bool mIntroElapsed = false;
    /* Startup text (IP address) was already shown */
    bool mStartupTextDone = false;
    /* At least one valid date/time was received */
    bool mDateTimeValid = false;

    /* Text to scroll (pending or in progress) */
    String  mScrollText;
    /* Text column displayed at the leftmost matrix column */
    int32_t mScrollOffset = 0;

    /* Leds */
    CRGB mLeds[LED_NUMBER];

    /* Display scene */
    DisplayScene mDisplayScene;

    /* Current date and time */
    DateTimeNS::tDateTime mDateTime;

    /* ApplicationNS::Task::ProcessIncomingMessage() */
    void ProcessIncomingMessage(const MessageNS::Message &arMessage) override;

    /* ApplicationNS::Task::ProcessTimerEvent() */
    void ProcessTimerEvent(const uint32_t aTimerId) override;

    void ShowIntro(void);

    void SetScrollText(const String& arText);
    void TryStartScrollText(void);
    void ScrollTextStep(void);
    void FinishScrollText(void);

    void Clear(void);
    void Fill(const CRGB aColor, const uint8_t aBrightness=100);

    void UpdateDisplay(const bool aForceRepaint = false);

    bool DisplayTimeScene(const bool aForceRepaint);

    bool IsDayMode(void) const;

    tColor ScaleColor(const tColor aColor, const uint8_t aBrightness) const;

    void SetLedColor(const uint16_t aLedIndex, const CRGB aColor);
    void SetLedColor(BitMatrix& arLedMask, const CRGB aColor);

    void PaintWord(const tWord aWord, const CRGB aColor);
    void PaintTime(const uint8_t aHour, const uint8_t aMinute, const CRGB aColor);
};
