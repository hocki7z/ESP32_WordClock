/*
 * Display.h
 *
 *  Created on: 02.09.2025
 *      Author: hocki
 */
#ifndef DISPLAY_H_
#define DISPLAY_H_

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

private:
    /* Leds */
    CRGB mLeds[LED_NUMBER];
    CRGB mTransitionFrom[LED_NUMBER];
    CRGB mTransitionTo[LED_NUMBER];

    /* Display scene */
    DisplayScene mDisplayScene;

    /* Current date and time */
    DateTimeNS::tDateTime mDateTime;

    /* ApplicationNS::Task::ProcessIncomingMessage() */
    void ProcessIncomingMessage(const MessageNS::Message &arMessage) override;

    void Clear(void);
    void Fill(const CRGB aColor, const uint8_t aBrightness=100);

    void UpdateDisplay(void);

    void DisplayTimeScene(void);

    void SetLedColor(const uint16_t aLedIndex, const CRGB aColor);
    void SetLedColor(BitMatrix& arLedMask, const CRGB aColor);

    void PaintWord(const tWord aWord, const CRGB aColor);
    void PaintTime(const uint8_t aHour, const uint8_t aMinute, const CRGB aColor);
    void NotifyWebPreview(void);
};

#endif /* DISPLAY_H_ */