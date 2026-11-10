/*
 * DisplayLayout.h
 *
 *  Created on: 19.10.2026
 *      Author: hocki
 */
#pragma once

#include <FastLED.h>
#include <MutexCPP.h>

#include "Application.h"

#include "DateTime.h"
#include "BitMatrix.h"

/** @brief Type representing a word index */
using tWord = uint8_t;

/** @brief 32-bit RGB color value in 0x00RRGGBB format */
using tColor = uint32_t;

/** @brief Mask selecting the 24 RGB color bits in 0x00RRGGBB format. */
#define COLOR_RGB_MASK              0x00FFFFFFu

// Conversion between FastLED CRGB and tColor:
//     CRGB → tColor (0x00RRGGBB)
//              tColor wColor = static_cast<tColor>(wRgbColor.as_uint32_t() & COLOR_RGB_MASK);
//     tColor → CRGB
//              CRGB wRgbColor(wColor & COLOR_RGB_MASK);

/** @brief Maximum number of words per group */
#define MAX_WORDS_PER_GROUP         5       // Main time needs at most 3 minute words + 2 hour words = 5


/** @brief Struct to store word data */
typedef struct tWordData
{
    /* Row index [0..MATRIX_HEIGHT-1] */
    uint8_t mRow;
    /* Column index [0..MATRIX_WIDTH-1] */
    uint8_t mColumn;
    /* Word length */
    uint8_t mLength;
} tWordData;

/** @brief Group of words with associated color and state */
struct WordGroup
{
    tColor   mColor = 0;
    uint8_t  mBrightness = 100;  // Brightness percentage (0-100)

    tWord    mWords[MAX_WORDS_PER_GROUP] = {};
    uint8_t  mWordsCount = 0;
    bool     mEnabled = false;

    void SetEnabled(bool aEnabled)
    {
        mEnabled = aEnabled;
    }

    bool IsEnabled(void) const
    {
        return mEnabled;
    }

    void Clear(void)
    {
        mColor = 0;
        mBrightness = 100;
        memset(mWords, 0, sizeof(mWords));
        mWordsCount = 0;
        mEnabled = false;
    }

    void CopyFrom(const WordGroup& arOther)
    {
        if (this != &arOther)
        {
            *this = arOther;
        }
    }

    bool operator==(const WordGroup& arOther) const
    {
        return (mColor == arOther.mColor) &&
               (mBrightness == arOther.mBrightness) &&
               (mWordsCount == arOther.mWordsCount) &&
               (mEnabled == arOther.mEnabled) &&
               (memcmp(mWords, arOther.mWords, sizeof(mWords)) == 0);
    }

    bool operator!=(const WordGroup& arOther) const
    {
        return !(*this == arOther);
    }
};

/** @brief Structure representing the entire display scene */
struct DisplayScene
{
    tColor   mColorBkgd      = 0;
    uint8_t  mBrightnessBkgd = 100;  // Brightness percentage (0-100)

    WordGroup mItIs;
    WordGroup mTime;
    WordGroup mExtraMinutes;

    void Clear(void)
    {
        mColorBkgd      = 0;
        mBrightnessBkgd = 100;

        mItIs.Clear();
        mTime.Clear();
        mExtraMinutes.Clear();
    }

    void CopyFrom(const DisplayScene& arOther)
    {
        if (this != &arOther)
        {
            *this = arOther;
        }
    }

    bool operator==(const DisplayScene& arOther) const
    {
        return (mColorBkgd == arOther.mColorBkgd) &&
               (mBrightnessBkgd == arOther.mBrightnessBkgd) &&
               (mItIs == arOther.mItIs) &&
               (mTime == arOther.mTime) &&
               (mExtraMinutes == arOther.mExtraMinutes);
    }

    bool operator!=(const DisplayScene& arOther) const
    {
        return !(*this == arOther);
    }
};

/** @brief Thread-safe owner of the latest display-scene snapshot (e.g. for the web preview) */
class DisplaySceneSnapshot
{
public:
    DisplaySceneSnapshot() : mMutex("DisplaySceneSnapshot")
    {
    }

    /** @brief Store a consistent snapshot of the scene
     *
     * @param arScene The scene to copy from
     * @return true if the snapshot was updated, false if the scene was unchanged
     */
    bool Store(const DisplayScene& arScene)
    {
        bool wUpdated = false;

        FreeRTOScpp::Lock wLock(mMutex);

        if (mScene != arScene)
        {
            mScene.CopyFrom(arScene);
            wUpdated = true;
        }

        return wUpdated;
    }

    /** @brief Copy the latest snapshot
     *
     * @param arScene The scene to copy into
     */
    void Get(DisplayScene& arScene) const
    {
        FreeRTOScpp::Lock wLock(mMutex);
        arScene.CopyFrom(mScene);
    }

private:
    /** @brief Mutex protecting the scene */
    mutable FreeRTOScpp::Mutex mMutex;

    /** @brief Latest scene snapshot */
    DisplayScene mScene;
};

/* Include the German 12-hour display layout */
#include "DisplayLayout_de_12h.h"