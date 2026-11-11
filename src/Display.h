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

    /* Fade the entire matrix to black or restore the latest time scene from black. */
    void SetDisplayEnabled(const bool aEnabled);

private:
    /* Fade directions */
    typedef enum tFadeDirection
    {
        FADE_OUT,
        FADE_HOLD,
        FADE_IN,
        FADE_TO_BLACK,
        FADE_BACKGROUND_IN,
        FADE_WORDS_IN,
        FADE_WORDS_OUT,
        FADE_BACKGROUND_OUT,
    } tFadeDirection;

    typedef enum tFadeMode
    {
        FADE_MODE_SCENE_CHANGE,
        FADE_MODE_INTRO_TO_SCENE,
        FADE_MODE_SCREEN_OFF,
        FADE_MODE_SCREEN_ON,
    } tFadeMode;


    /* Leds */
    CRGB mLeds[LED_NUMBER];
    CRGB mDisplayFrame[LED_NUMBER];
    bool mDisplayFrameValid = false;
    bool mDisplayEnabled = true;

    /* Fade transition frames, background, and current phase start time */
    CRGB     mFadeFrom[LED_NUMBER];
    CRGB     mFadeTo[LED_NUMBER];
    CRGB     mFadeBkgd;
    uint32_t mFadeStart  = 0;
    /* Fade phase durations; all LEDs in a phase animate together. */
    uint32_t mFadeOutDurationMs = 0;
    uint32_t mFadeInDurationMs  = 0;
    bool     mFadeActive = false;
    uint32_t mFadeStepPeriodMs = 40;
    uint8_t  mFadeBlendIncrement = 6;
    tFadeDirection mFadeDirection = FADE_OUT;
    tFadeMode mFadeMode = FADE_MODE_SCENE_CHANGE;
    /* Previous and incoming word LED masks. */
    BitMatrix mFadeOutMask{MATRIX_WIDTH, MATRIX_HEIGHT};
    BitMatrix mFadeInMask{MATRIX_WIDTH, MATRIX_HEIGHT};

    /* Display scene */
    DisplayScene mDisplayScene;

    /* Current date and time */
    DateTimeNS::tDateTime mDateTime;

    /* Override ApplicationNS::Task::ProcessIncomingMessage() */
    void ProcessIncomingMessage(const MessageNS::Message &arMessage) override;
    /* Override ApplicationNS::Task::ProcessTimerEvent() */
    void ProcessTimerEvent(const uint32_t aTimerId = 0) override;

    void Clear(void);
    void Fill(const CRGB aColor, const uint8_t aBrightness=100);

    void UpdateDisplay(void);

    bool DisplayTimeScene(void);

    bool IsDayMode(void) const;

    tColor ScaleColor(const tColor aColor, const uint8_t aBrightness) const;

    void SetLedColor(const uint16_t aLedIndex, const CRGB aColor);
    void SetLedColor(BitMatrix& arLedMask, const CRGB aColor);

    void PaintWord(const tWord aWord, const CRGB aColor);
    void AddGroupToMask(BitMatrix& arMask, const WordGroup& arGroup) const;

    void StartFade(void);
    void StartBlackFade(const tFadeMode aMode);
    void FadeStep(void);
    void StopFade(void);
};
