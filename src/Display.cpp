/*
 * Display.h
 *
 *  Created on: 02.09.2025
 *      Author: hocki
 */

#include <WiFi.h>

#include "Global.h"
#include "Display.h"
#include "Font5x7.h"


/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)


/* Delay in msec between display updates */
static constexpr uint32_t mcUpdateDelay = 10;

/* Intro color */
static constexpr tColor mIntroColor = 0xFFA500;     // CRGB::Orange

/* Duration of the intro before the startup text is scrolled */
static constexpr uint32_t mcIntroDelayMs = 3000;
/* Delay between two scroll steps (one matrix column) */
static constexpr uint32_t mcScrollStepMs = 70;
/* Scrolling text color */
static constexpr tColor mScrollTextColor = mIntroColor;
/* Top row of the scrolling text (vertically centered) */
static constexpr uint8_t mcScrollTextRow = (MATRIX_HEIGHT - Font5x7NS::mGlyphHeight) / 2;
/* Width of one character including spacing */
static constexpr int32_t mcScrollCharWidth = Font5x7NS::mGlyphWidth + Font5x7NS::mGlyphSpacing;

/* Time change: fade out old time to background, then fade in new time */
static constexpr uint32_t mcFadeOutMs  = 2000;
static constexpr uint32_t mcFadeInMs   = 2000;
/* Delay between two fade steps */
static constexpr uint32_t mcFadeStepMs = 20;

/* Colortwinkles timing with independent saver speed/intensity, applied to whole words.
 * No fixed hold: brighten to saturation, decay to black, then randomly restart.
 * WLED-MM / Effects1D.cpp (EUPL-1.2) uses a 24 ms frame time.
 */
static constexpr uint32_t mcSaverStepMs     = 24;

/* Effects: delay between two frames */
static constexpr uint32_t mcEffectStepMs    = 25;


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
    if (mpIntroTimer)
    {
        mpIntroTimer->stop();
        delete mpIntroTimer;
        mpIntroTimer = nullptr;
    }

    if (mpScrollTimer)
    {
        mpScrollTimer->stop();
        delete mpScrollTimer;
        mpScrollTimer = nullptr;
    }

    if (mpFadeTimer)
    {
        mpFadeTimer->stop();
        delete mpFadeTimer;
        mpFadeTimer = nullptr;
    }

    if (mpSaverTimer)
    {
        mpSaverTimer->stop();
        delete mpSaverTimer;
        mpSaverTimer = nullptr;
    }

    if (mpEffectTimer)
    {
        mpEffectTimer->stop();
        delete mpEffectTimer;
        mpEffectTimer = nullptr;
    }
}

void Display::Init(ApplicationNS::tTaskObjects* apTaskObjects)
{
    /* Initialize base class */
    ApplicationNS::Task::Init(apTaskObjects);

    /* Create intro (one-shot) and scroll (periodic) timers */
    mTimerObjects.mTaskHandle = this->getTaskHandle();
    mTimerObjects.mpTaskMessagesQueue = this->mpTaskObjects->mpMessageQueue;

    mpIntroTimer = new ApplicationNS::TaskTimer(mIntroTimerId, pdMS_TO_TICKS(mcIntroDelayMs), false);
    mpIntroTimer->Init(&mTimerObjects);

    mpScrollTimer = new ApplicationNS::TaskTimer(mScrollTimerId, pdMS_TO_TICKS(mcScrollStepMs), true);
    mpScrollTimer->Init(&mTimerObjects);

    mpFadeTimer = new ApplicationNS::TaskTimer(mFadeTimerId, pdMS_TO_TICKS(mcFadeStepMs), true);
    mpFadeTimer->Init(&mTimerObjects);

    mpSaverTimer = new ApplicationNS::TaskTimer(mSaverTimerId, pdMS_TO_TICKS(mcSaverStepMs), true);
    mpSaverTimer->Init(&mTimerObjects);

    mpEffectTimer = new ApplicationNS::TaskTimer(mEffectTimerId, pdMS_TO_TICKS(mcEffectStepMs), true);
    mpEffectTimer->Init(&mTimerObjects);

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
    ShowIntro();
}

void Display::task(void)
{
    /* Start intro delay */
    mState = STATE_INTRO;
    mpIntroTimer->start();

    /* Execute base class task */
    ApplicationNS::Task::task();
}

void Display::ProcessTimerEvent(const uint32_t aTimerId)
{
    switch (aTimerId)
    {
        case mIntroTimerId:
            LOG(LOG_DEBUG, "Display::ProcessTimerEvent() Intro delay elapsed");
            mIntroElapsed = true;

            if (Settings.GetValue<bool>(ConfigNS::mKeyDisplayStartupText, ConfigNS::mDefaultDisplayStartupText))
            {
                TryStartScrollText();
            }
            else
            {
                /* Startup text disabled: go directly to time display */
                FinishScrollText();
            }
            break;

        case mScrollTimerId:
            ScrollTextStep();
            break;

        case mFadeTimerId:
            FadeStep();
            break;

        case mSaverTimerId:
            ScreenSaverStep();
            break;

        case mEffectTimerId:
            EffectStep();
            break;

        default:
            LOG(LOG_ERROR, "Display::ProcessTimerEvent() Unknown timer ID %08X", aTimerId);
            break;
    }
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
                mDateTimeValid = true;

                LOG(LOG_DEBUG, "Display::ProcessIncomingMessage() Datetime changed: " PRINTF_DATETIME_PATTERN,
                        PRINTF_DATETIME_FORMAT(mDateTime));

                /* Update display (time is shown only after the startup sequence) */
                if (mState == STATE_TIME)
                {
                    UpdateDisplay(false, true);
                }
                else if (mState == STATE_EFFECT)
                {
                    /* Time text and day/night brightness may have changed */
                    UpdateEffectText();
                    LoadEffectSettings();
                }
            }
        }
            break;

        case MessageNS::tMessageId::MSG_EVENT_SETTINGS_CHANGED:
        {
            // Settings changed, re-read settings if needed
            LOG(LOG_DEBUG, "Display::ProcessIncomingMessage() Settings changed");

            /* Switch mode or update the current one (not during the startup sequence) */
            if ((mState == STATE_TIME) || (mState == STATE_SCREEN_SAVER) || (mState == STATE_EFFECT))
            {
                SwitchMode();
            }
        }
            break;

        case MessageNS::tMessageId::MSG_EVENT_WIFI_STA_GOT_IP:
            if (!mStartupTextDone)
            {
                SetScrollText(String("IP ") + WiFi.localIP().toString());
            }
            break;

        case MessageNS::tMessageId::MSG_EVENT_WIFI_AP_STARTED:
            if (!mStartupTextDone)
            {
                SetScrollText(String("AP ") + WiFi.softAPIP().toString());
            }
            break;

        default:
            // do nothing
            break;
    }
}

/**
 * @brief Shows the intro word on an otherwise dark matrix.
 */
void Display::ShowIntro(void)
{
    Clear();
    PaintWord(WORD_WORDCLOCK, mIntroColor & COLOR_RGB_MASK);

    /* Show new data on the LED matrix */
    FastLED.show();
}

/**
 * @brief Sets the text to be scrolled and starts scrolling when possible.
 *
 * @param arText The text to scroll.
 */
void Display::SetScrollText(const String& arText)
{
    LOG(LOG_DEBUG, "Display::SetScrollText() '%s'", arText.c_str());

    mScrollText = arText;
    TryStartScrollText();
}

/**
 * @brief Starts scrolling the pending text once the intro delay has elapsed.
 */
void Display::TryStartScrollText(void)
{
    if ((mState == STATE_INTRO) && mIntroElapsed && (mScrollText.length() > 0))
    {
        /* Start with the text just outside the right matrix edge */
        mScrollOffset = -static_cast<int32_t>(MATRIX_WIDTH);
        mState = STATE_SCROLL_TEXT;

        ScrollTextStep();
        mpScrollTimer->start();
    }
}

/**
 * @brief Renders one scroll frame and advances the text by one column.
 */
void Display::ScrollTextStep(void)
{
    if (mState != STATE_SCROLL_TEXT)
    {
        return;
    }

    const int32_t wTextLength = static_cast<int32_t>(mScrollText.length());
    const int32_t wTextWidth  = wTextLength * mcScrollCharWidth;

    /* Text has completely left the matrix */
    if (mScrollOffset > wTextWidth)
    {
        FinishScrollText();
        return;
    }

    BitMatrix wTextMask(MATRIX_WIDTH, MATRIX_HEIGHT);

    for (uint16_t wCol = 0; wCol < MATRIX_WIDTH; wCol++)
    {
        const int32_t wTextCol = mScrollOffset + wCol;

        if ((wTextCol >= 0) && (wTextCol < wTextWidth))
        {
            const int32_t wCharIndex = wTextCol / mcScrollCharWidth;
            const int32_t wGlyphCol  = wTextCol % mcScrollCharWidth;

            if (wGlyphCol < Font5x7NS::mGlyphWidth)
            {
                const uint8_t wBits = Font5x7NS::GetGlyph(mScrollText[wCharIndex])[wGlyphCol];

                for (uint8_t wRow = 0; wRow < Font5x7NS::mGlyphHeight; wRow++)
                {
                    if ((wBits & (1U << wRow)) != 0)
                    {
                        wTextMask.SetBit(mcScrollTextRow + wRow, wCol);
                    }
                }
            }
        }
    }

    /* Match the logical LED mask to the physical layout (zigzag order) */
    wTextMask.SetZigzagOrder(true);

    Clear();
    SetLedColor(wTextMask, CRGB(mScrollTextColor & COLOR_RGB_MASK));
    FastLED.show();

    mScrollOffset++;
}

/**
 * @brief Ends scrolling and switches to the time display.
 */
void Display::FinishScrollText(void)
{
    mpScrollTimer->stop();

    mScrollText = "";
    mStartupTextDone = true;

    LOG(LOG_DEBUG, "Display::FinishScrollText() Startup sequence finished");

    /* Current state is intro or scroll text, so the requested mode is always entered */
    SwitchMode();
}

/**
 * @brief Returns the display mode selected in the settings.
 */
Display::tDisplayState Display::GetRequestedMode(void) const
{
    if (Settings.GetValue<bool>(ConfigNS::mKeyDisplayEffectMode, ConfigNS::mDefaultDisplayEffectMode))
    {
        return STATE_EFFECT;
    }
    if (Settings.GetValue<bool>(ConfigNS::mKeyDisplayScreenSaver, ConfigNS::mDefaultDisplayScreenSaver))
    {
        return STATE_SCREEN_SAVER;
    }
    return STATE_TIME;
}

/**
 * @brief Enters the requested display mode, or refreshes the current one if unchanged.
 */
void Display::SwitchMode(void)
{
    const tDisplayState wTarget = GetRequestedMode();

    if (wTarget == mState)
    {
        if ((mState == STATE_TIME) && mDateTimeValid)
        {
            UpdateDisplay();
        }
        else if (mState == STATE_EFFECT)
        {
            LoadEffectSettings();
        }
        else if (mState == STATE_SCREEN_SAVER)
        {
            LoadScreenSaverSettings();
        }
        return;
    }

    LOG(LOG_DEBUG, "Display::SwitchMode() %d -> %d", mState, wTarget);

    StopFade();
    mpSaverTimer->stop();
    mpEffectTimer->stop();

    switch (wTarget)
    {
        case STATE_SCREEN_SAVER:
            StartScreenSaver();
            break;

        case STATE_EFFECT:
            StartEffect();
            break;

        default:
            mState = STATE_TIME;
            /* Empty scene makes UpdateDisplay() fade in all parts from black */
            mDisplayScene.Clear();
            if (mDateTimeValid)
            {
                UpdateDisplay(true, true);
            }
            else
            {
                /* No time received yet (e.g. AP mode): keep intro until the first time update */
                ShowIntro();
            }
            break;
    }
}

/**
 * @brief Starts the selected LED effect from a black screen.
 */
void Display::StartEffect(void)
{
    mState = STATE_EFFECT;

    Clear();
    FastLED.show();

    mEffects.Start(Settings.GetValue<uint8_t>(ConfigNS::mKeyDisplayEffect, ConfigNS::mDefaultDisplayEffect),
                   Settings.GetValue<uint8_t>(ConfigNS::mKeyDisplayEffectMapping, ConfigNS::mDefaultDisplayEffectMapping));
    LoadEffectSettings();
    UpdateEffectText();

    mpEffectTimer->start();
}

/**
 * @brief Applies effect selection, speed, intensity, palette and brightness from the settings.
 */
void Display::LoadEffectSettings(void)
{
    const uint8_t wEffect  = Settings.GetValue<uint8_t>(ConfigNS::mKeyDisplayEffect, ConfigNS::mDefaultDisplayEffect);
    const uint8_t wMapping = Settings.GetValue<uint8_t>(ConfigNS::mKeyDisplayEffectMapping, ConfigNS::mDefaultDisplayEffectMapping);
    if ((wEffect != mEffects.GetEffect()) || (wMapping != mEffects.GetMapping()))
    {
        mEffects.Start(wEffect, wMapping);
    }

    mEffects.SetColors(
        CRGB(Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayEffectColor1, ConfigNS::mDefaultDisplayEffectColor1) & COLOR_RGB_MASK),
        CRGB(Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayEffectColor2, ConfigNS::mDefaultDisplayEffectColor2) & COLOR_RGB_MASK),
        CRGB(Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayEffectColor3, ConfigNS::mDefaultDisplayEffectColor3) & COLOR_RGB_MASK));

    const uint8_t wSpeed     = Settings.GetValue<uint8_t>(ConfigNS::mKeyDisplayEffectSpeed, ConfigNS::mDefaultDisplayEffectSpeed);
    const uint8_t wIntensity = Settings.GetValue<uint8_t>(ConfigNS::mKeyDisplayEffectIntensity, ConfigNS::mDefaultDisplayEffectIntensity);
    const uint8_t wPalette   = Settings.GetValue<uint8_t>(ConfigNS::mKeyDisplayEffectPalette, ConfigNS::mDefaultDisplayEffectPalette);

    /* UI sliders are percentages */
    mEffects.SetParams(map(std::min<uint8_t>(wSpeed, 100), 0, 100, 0, 255),
                       map(std::min<uint8_t>(wIntensity, 100), 0, 100, 0, 255),
                       wPalette);

    /* Effects use the time brightness of the current day/night mode */
    const uint8_t wBrightness = IsDayMode()
        ? Settings.GetValue<uint8_t>(ConfigNS::mKeyDisplayBrightnessTime, ConfigNS::mDefaultDisplayBrightnessTime)
        : Settings.GetValue<uint8_t>(ConfigNS::mKeyDisplayBrightnessTimeNight, ConfigNS::mDefaultDisplayBrightnessTimeNight);
    mEffectBrightness = map(std::min<uint8_t>(wBrightness, 100), 0, 100, 0, 255);
}

/**
 * @brief Sets the current time as text for the scrolling text effect.
 */
void Display::UpdateEffectText(void)
{
    char wText[8];

    if (mDateTimeValid)
    {
        snprintf(wText, sizeof(wText), "%02u:%02u", mDateTime.mTime.mHour, mDateTime.mTime.mMinute);
    }
    else
    {
        snprintf(wText, sizeof(wText), "--:--");
    }
    mEffects.SetText(wText);
}

/**
 * @brief Renders one effect frame and shows it on the LED matrix.
 */
void Display::EffectStep(void)
{
    if (mState != STATE_EFFECT)
    {
        return;
    }

    mEffects.Render(millis());

    for (int wY = 0; wY < MATRIX_HEIGHT; wY++)
    {
        for (int wX = 0; wX < MATRIX_WIDTH; wX++)
        {
            CRGB wColor = mEffects.GetPixel(wX, wY);
            wColor.nscale8_video(mEffectBrightness);

            /* Even rows are mirrored like BitMatrix::SetZigzagOrder(true) */
            const int wCol = ((wY % 2) == 0) ? (MATRIX_WIDTH - 1 - wX) : wX;
            mLeds[wY * MATRIX_WIDTH + wCol] = wColor;
        }
    }

    FastLED.show();
}

/**
 * @brief Starts the screen saver from a black screen.
 */
void Display::StartScreenSaver(void)
{
    LOG(LOG_DEBUG, "Display::StartScreenSaver()");

    StopFade();
    mState = STATE_SCREEN_SAVER;

    for (tSaverWord& wSlot : mSaverWords)
    {
        wSlot.mWord = WORD_END_OF_WORDS;
    }

    Clear();
    FastLED.show();

    LoadScreenSaverSettings();
    for (bool& wFadeUp : mSaverLetterFadeUp) wFadeUp = false;
    mpSaverTimer->start();
}

/**
 * @brief Loads saver speed, intensity and palette, sharing custom colors with effects.
 */
void Display::LoadScreenSaverSettings(void)
{
    const uint8_t wPresentation = Settings.GetValue<uint8_t>(
        ConfigNS::mKeyDisplayScreenSaverPresentation, ConfigNS::mDefaultDisplayScreenSaverPresentation);
    const bool wLetters = (wPresentation == 1);
    if (wLetters != mSaverSeparateLetters)
    {
        mSaverSeparateLetters = wLetters;
        for (tSaverWord& wSlot : mSaverWords) wSlot.mWord = WORD_END_OF_WORDS;
        for (bool& wFadeUp : mSaverLetterFadeUp) wFadeUp = false;
        Clear();
    }

    const uint8_t wSpeed = Settings.GetValue<uint8_t>(
        ConfigNS::mKeyDisplayScreenSaverSpeed, ConfigNS::mDefaultDisplayScreenSaverSpeed);
    const uint8_t wIntensity = Settings.GetValue<uint8_t>(
        ConfigNS::mKeyDisplayScreenSaverIntensity, ConfigNS::mDefaultDisplayScreenSaverIntensity);
    mSaverSpeed = map(std::min<uint8_t>(wSpeed, 100), 0, 100, 0, 255);
    mSaverIntensity = map(std::min<uint8_t>(wIntensity, 100), 0, 100, 0, 255);

    const CRGB wColors[3] = {
        CRGB(Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayEffectColor1, ConfigNS::mDefaultDisplayEffectColor1) & COLOR_RGB_MASK),
        CRGB(Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayEffectColor2, ConfigNS::mDefaultDisplayEffectColor2) & COLOR_RGB_MASK),
        CRGB(Settings.GetValue<uint32_t>(ConfigNS::mKeyDisplayEffectColor3, ConfigNS::mDefaultDisplayEffectColor3) & COLOR_RGB_MASK)
    };
    mSaverPalette = EffectsNS::GetPalette(
        Settings.GetValue<uint8_t>(ConfigNS::mKeyDisplayScreenSaverPalette, ConfigNS::mDefaultDisplayScreenSaverPalette), wColors);
}

/**
 * @brief Applies the saver envelope to whole words or separate letters.
 */
void Display::ScreenSaverStep(void)
{
    if (mState != STATE_SCREEN_SAVER)
    {
        return;
    }

    if (mSaverSeparateLetters)
    {
        ScreenSaverLettersStep();
        return;
    }

    for (tSaverWord& wSlot : mSaverWords)
    {
        if (wSlot.mWord == WORD_END_OF_WORDS)
        {
            continue;
        }

        EffectsNS::StepColorTwinkle(wSlot.mColor, wSlot.mFadeUp, mSaverSpeed);
        if (wSlot.mColor == CRGB(CRGB::Black)) wSlot.mWord = WORD_END_OF_WORDS;
    }

    /* As with Colortwinkles on a short strip: one random ignition trial/frame.
     * Only one word can start in a frame; occupied slots remain untouched.
     */
    if (random8() <= mSaverIntensity) SpawnSaverWord();

    Clear();
    for (const tSaverWord& wSlot : mSaverWords)
    {
        if (wSlot.mWord == WORD_END_OF_WORDS) continue;

        /* Direct LED indexing; even rows are mirrored like BitMatrix::SetZigzagOrder(true) */
        const tWordData& wData = mcWordDataArray[wSlot.mWord];
        for (uint8_t wC = wData.mColumn; wC < (wData.mColumn + wData.mLength); wC++)
        {
            const uint8_t wCol = ((wData.mRow % 2) == 0) ? (MATRIX_WIDTH - 1 - wC) : wC;
            SetLedColor(static_cast<uint16_t>(wData.mRow * MATRIX_WIDTH + wCol), wSlot.mColor);
        }
    }

    FastLED.show();
}

/**
 * @brief Twinkles individual letters without invoking the effects engine.
 */
void Display::ScreenSaverLettersStep(void)
{
    for (uint16_t wI = 0; wI < LED_NUMBER; wI++)
    {
        EffectsNS::StepColorTwinkle(mLeds[wI], mSaverLetterFadeUp[wI], mSaverSpeed);
    }

    /* Colortwinkles scales ignition attempts with the number of pixels. */
    for (uint16_t wTrial = 0; wTrial <= LED_NUMBER / 50; wTrial++)
    {
        if (random8() > mSaverIntensity) continue;
        for (uint8_t wAttempt = 0; wAttempt < 5; wAttempt++)
        {
            const uint16_t wIndex = random16(LED_NUMBER);
            if (mLeds[wIndex] != CRGB(CRGB::Black)) continue;
            const CRGB wColor = ColorFromPalette(mSaverPalette, random8(), 64, NOBLEND);
            if (wColor == CRGB(CRGB::Black)) continue;
            mLeds[wIndex] = wColor;
            mSaverLetterFadeUp[wIndex] = true;
            break;
        }
    }
    FastLED.show();
}

/**
 * @brief Places a random, non-overlapping word with a random color into a free slot.
 */
void Display::SpawnSaverWord(void)
{
    /* Like Colortwinkles, sample up to five random positions for an off slot.
     * This supplies random off time rather than a fixed spawn interval.
     */
    for (uint8_t wAttempt = 0; wAttempt < 5; wAttempt++)
    {
        tSaverWord& wSlot = mSaverWords[random8(mcSaverMaxWords)];
        if (wSlot.mWord == WORD_END_OF_WORDS)
        {
            /* A few attempts to find a word not overlapping the visible ones */
            for (uint8_t wTry = 0; wTry < 10; wTry++)
            {
                const tWord wWord = static_cast<tWord>(random(WORD_END_OF_WORDS + 1, WORD_MAX_NUMBER));

                if (IsSaverWordFree(wWord))
                {
                    const CRGB wColor = ColorFromPalette(mSaverPalette, random8(), 64, NOBLEND);
                    if (wColor == CRGB(CRGB::Black)) continue;
                    wSlot.mWord  = wWord;
                    wSlot.mColor = wColor;
                    wSlot.mFadeUp = true;
                    break;
                }
            }
            return;
        }
    }
}

/**
 * @brief Checks that a word does not share LEDs with any visible screen saver word.
 */
bool Display::IsSaverWordFree(const tWord aWord) const
{
    const tWordData& wData = mcWordDataArray[aWord];

    for (const tSaverWord& wSlot : mSaverWords)
    {
        if (wSlot.mWord != WORD_END_OF_WORDS)
        {
            const tWordData& wOther = mcWordDataArray[wSlot.mWord];

            if ((wOther.mRow == wData.mRow) &&
                (wData.mColumn  < (wOther.mColumn + wOther.mLength)) &&
                (wOther.mColumn < (wData.mColumn  + wData.mLength)))
            {
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Clears the LED matrix by filling it with black color.
 */
void Display::Clear(void)
{
    Fill(CRGB::Black);
}

/**
 * @brief Fills the LED matrix with the specified color and brightness.
 *
 * @param aColor The color to fill the matrix with.
 * @param aBrightness The brightness level (0-100), default is 100.
 */
void Display::Fill(const CRGB aColor, const uint8_t aBrightness)
{
    uint8_t wBrightness = (aBrightness > 100) ? 100 : aBrightness;
    uint8_t wAlphaScale = map(wBrightness, 0, 100, 0, 255);

    fill_solid(mLeds, LED_NUMBER, aColor.scale8(wAlphaScale));
}

/**
 * @brief Updates the LED matrix display with the current scene.
 *
 * @param aForceRepaint Repaint the LEDs even if the scene is unchanged.
 * @param aFade Fade from the current frame to the new scene instead of switching at once.
 */
void Display::UpdateDisplay(const bool aForceRepaint, const bool aFade)
{
    /* LOG */
    LOG(LOG_DEBUG, "Display.UpdateDisplay() Update display for time %02u:%02u",
            mDateTime.mTime.mHour,  mDateTime.mTime.mMinute);

    /* An interrupted fade leaves an intermediate frame, so the scene must be repainted */
    const bool wFadeInterrupted = StopFade();

    /* No word group shown yet (after intro or scrolling text) */
    const DisplayScene wOldScene = mDisplayScene;
    const bool wInitial = !wOldScene.mItIs.IsEnabled() &&
                          !wOldScene.mTime.IsEnabled() &&
                          !wOldScene.mExtraMinutes.IsEnabled();

    if (aFade)
    {
        if (wInitial)
        {
            Clear();
        }

        memcpy(mFadeFrom, mLeds, sizeof(mLeds));
    }

    /* Prepare time scene */
    bool wUpdated = DisplayTimeScene(aForceRepaint || wFadeInterrupted);

    /* Display scene changed */
    if (wUpdated)
    {
        if (aFade)
        {
            /* Changed time words imply new extra minutes too */
            const bool wItIsChanged  = (wOldScene.mItIs != mDisplayScene.mItIs);
            const bool wTimeChanged  = (wOldScene.mTime != mDisplayScene.mTime);
            const bool wExtraChanged = wTimeChanged || (wOldScene.mExtraMinutes != mDisplayScene.mExtraMinutes);

            auto wAddGroups = [this](const WordGroup& arOld, const WordGroup& arNew)
            {
                AddGroupToMask(mFadeMask, arOld);
                AddGroupToMask(mFadeMask, arNew);
            };

            mFadeMask.ClearAll();
            if (wItIsChanged)
            {
                wAddGroups(wOldScene.mItIs, mDisplayScene.mItIs);
            }
            if (wTimeChanged)
            {
                wAddGroups(wOldScene.mTime, mDisplayScene.mTime);
            }
            if (wExtraChanged)
            {
                wAddGroups(wOldScene.mExtraMinutes, mDisplayScene.mExtraMinutes);
            }
            mFadeMask.SetZigzagOrder(true);

            StartFade(wInitial ? 0 : mcFadeOutMs, mcFadeInMs);
        }
        else
        {
            /* Show new data on the LED matrix */
            FastLED.show();
        }

        /* Publish a compact snapshot for the web UI */
        MessageNS::Message wMessage{};
        wMessage.mSource = MessageNS::tAddress::DISPLAY_MANAGER;
        wMessage.mDestination = MessageNS::tAddress::WEB_MANAGER;
        wMessage.mId = MessageNS::tMessageId::MSG_EVENT_DISPLAY_FRAME_READY;

        SendMessage(wMessage);
    }
}

/**
 * @brief Starts the fade from mFadeFrom to the freshly rendered scene in mLeds.
 *
 * @param aOutDurationMs Fade-out duration (0 = fade in directly from the current frame).
 * @param aInDurationMs  Fade-in duration.
 */
void Display::StartFade(const uint32_t aOutDurationMs, const uint32_t aInDurationMs)
{
    memcpy(mFadeTo, mLeds, sizeof(mLeds));
    memcpy(mLeds, mFadeFrom, sizeof(mLeds));

    mFadeBkgd   = CRGB(ScaleColor(mDisplayScene.mColorBkgd, mDisplayScene.mBrightnessBkgd) & COLOR_RGB_MASK);
    mFadeOutDurationMs = aOutDurationMs;
    mFadeInDurationMs  = aInDurationMs;
    mFadeStart  = millis();
    mFadeActive = true;

    mpFadeTimer->start();
}

/**
 * @brief Renders one fade frame.
 *
 * @details Masked LEDs fade old color -> background, then background -> new color.
 *          Other changed LEDs (e.g. background) blend old -> new during fade-in.
 */
void Display::FadeStep(void)
{
    if (!mFadeActive)
    {
        return;
    }

    const uint32_t wElapsed = millis() - mFadeStart;

    if (wElapsed >= (mFadeOutDurationMs + mFadeInDurationMs))
    {
        memcpy(mLeds, mFadeTo, sizeof(mLeds));
        StopFade();
    }
    else if (wElapsed < mFadeOutDurationMs)
    {
        const uint8_t wAmount = EffectsNS::FadeAmount(wElapsed, mFadeOutDurationMs);

        for (uint16_t wI = 0; wI < LED_NUMBER; wI++)
        {
            if (mFadeMask.IsBitSet(wI))
            {
                mLeds[wI] = EffectsNS::FadeColor(mFadeFrom[wI], mFadeBkgd, wAmount);
            }
        }
    }
    else
    {
        const uint8_t wAmount = EffectsNS::FadeAmount(wElapsed - mFadeOutDurationMs, mFadeInDurationMs);

        for (uint16_t wI = 0; wI < LED_NUMBER; wI++)
        {
            if (mFadeMask.IsBitSet(wI))
            {
                const CRGB wStart = (mFadeOutDurationMs > 0) ? mFadeBkgd : mFadeFrom[wI];
                mLeds[wI] = EffectsNS::FadeColor(wStart, mFadeTo[wI], wAmount);
            }
            else if (mFadeFrom[wI] != mFadeTo[wI])
            {
                mLeds[wI] = EffectsNS::FadeColor(mFadeFrom[wI], mFadeTo[wI], wAmount);
            }
        }
    }

    FastLED.show();
}

/**
 * @brief Stops a running fade.
 *
 * @return true if a fade was running.
 */
bool Display::StopFade(void)
{
    const bool wWasActive = mFadeActive;

    mpFadeTimer->stop();
    mFadeActive = false;

    return wWasActive;
}

/**
 * @brief Adds the LEDs of an enabled word group to a logical (non-zigzag) mask.
 */
void Display::AddGroupToMask(BitMatrix& arMask, const WordGroup& arGroup) const
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
                arMask.SetLine(wData.mRow, wData.mColumn, wData.mLength);
            }
        }
    }
}

/**
 * @brief Builds and displays the scene for the current time.
 *
 * @details Reads display settings, creates the word groups for the current
 *          time, optionally logs the scene, then renders each enabled group
 *          using its configured color.
 *
 * @param aForceRepaint Repaint the LEDs even if the scene is unchanged.
 */
bool Display::DisplayTimeScene(const bool aForceRepaint)
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
        memcpy(&mDisplayScene.mTime.mWords[0], wMinuteDisplay.wMinuteWords,
            sizeof(wMinuteDisplay.wMinuteWords));

        /* Add hour words to displayed time */
        memcpy(&mDisplayScene.mTime.mWords[MAX_MINUTE_WORDS], mcWordHoursTable[wMinuteDisplay.mHourMode][wHour],
            sizeof(mcWordHoursTable[wMinuteDisplay.mHourMode][wHour]));

        mDisplayScene.mTime.mWordsCount = MAX_MINUTE_WORDS + MAX_HOUR_WORDS;

        /* Add extra minute words */
        memcpy(&mDisplayScene.mExtraMinutes.mWords[0], mcWordExtraMinutesTable[wMinuteExtra],
            sizeof(mcWordExtraMinutesTable[wMinuteExtra]));

        mDisplayScene.mExtraMinutes.mWordsCount = MAX_EXTRA_MINUTE_WORDS;
    }

    /* Store the display scene snapshot for the web UI */
    wUpdated = mDisplaySceneSnapshot.Store(mDisplayScene) || aForceRepaint;

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

/**
 * @brief Checks if the display is in day mode.
 * @details This function checks the current time against the configured day mode start and end times.
 *          If day/night mode is disabled, it will always return true (day mode).
 *
 * @return True if the display is in day mode, false otherwise.
 */
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

/**
 * @brief Scales the given color based on the specified brightness percentage.
 *
 * @param aColor The original color to be scaled.
 * @param aBrightness The brightness percentage (0-100) to scale the color.
 * @return The scaled color as a tColor value.
 */
tColor Display::ScaleColor(const tColor aColor, const uint8_t aBrightness) const
{
    uint8_t wBrightness = (aBrightness > 100) ? 100 : aBrightness;
    uint8_t wAlphaScale = map(wBrightness, 0, 100, 0, 255);

    CRGB wLedColor = CRGB(aColor & COLOR_RGB_MASK);
    wLedColor.nscale8(wAlphaScale);

    return static_cast<tColor>(wLedColor.as_uint32_t() & COLOR_RGB_MASK);
}

/**
 * @brief Sets the color of a specific LED in the matrix.
 *
 * @param aLedIndex The index of the LED to set.
 * @param aColor The color to set the LED to.
 */
void Display::SetLedColor(const uint16_t aLedIndex, const CRGB aColor)
{
    if (aLedIndex < LED_NUMBER)
    {
        mLeds[aLedIndex] = aColor;
    }
}

/**
 * @brief Sets the color of LEDs in the matrix based on a bitmask.
 *
 * @param arLedMask The bitmask representing which LEDs to set.
 * @param aColor The color to set the LEDs to.
 */
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

/**
 * @brief Paints a specific word on the LED matrix with the given color.
 *
 * @param aWord The word to paint.
 * @param aColor The color to paint the word with.
 */
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
