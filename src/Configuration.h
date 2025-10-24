/*
 * Configuration.h
 *
 *  Created on: 22.10.2025
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>

#include "Application.h"
#include "Settings.hpp"


/* Imports all symbols from the ApplicationNS namespace */
using namespace ApplicationNS;


namespace ConfigNS
{
    /**
     * Task configurations
     */

    /** @brief Default task stack size of tasks */
    static constexpr uint32_t    mDefaultTaskStackSize       = 2 * 1024;
    /** @brief Default priority of tasks */
    static constexpr tTaskPriority mDefaultTaskPriority      = FreeRTOScpp::TaskPrio_Low;

    /* Display task configuration */
    static constexpr tTaskPriority mDisplayTaskPriority      = mDefaultTaskPriority;
    static constexpr uint32_t      mDisplayTaskStackSize     = mDefaultTaskStackSize;
    static constexpr const char*   mDisplayTaskName          = "DisplayTask";

    /* Time manager task configuration */
    static constexpr tTaskPriority mTimeManagerTaskPriority  = mDefaultTaskPriority;
    static constexpr uint32_t      mTimeManagerTaskStackSize = mDefaultTaskStackSize;
    static constexpr const char*   mTimeManagerTaskName      = "TimeManagerTask";

    /* WiFi manager task configuration */
    static constexpr tTaskPriority mWifiManagerTaskPriority  = mDefaultTaskPriority;
    static constexpr uint32_t      mWifiManagerTaskStackSize = mDefaultTaskStackSize;
    static constexpr const char*   mWifiManagerTaskName      = "WifiManagerTask";

    /* Web site task configuration */
    static constexpr tTaskPriority mWebSiteTaskPriority      = mDefaultTaskPriority;
    static constexpr uint32_t      mWebSiteTaskStackSize     = mDefaultTaskStackSize;
    static constexpr const char*   mWebSiteTaskName          = "WebSiteTask";


    /**
     * Settings configurations
     */

    /* Keys for reset counters */
    static constexpr SettingsNS::tKey mKeyCounterResetPowerOn       = "CNT_RST_PWRON";
    static constexpr SettingsNS::tKey mKeyCounterResetSoftware      = "CNT_RST_SW";
    static constexpr SettingsNS::tKey mKeyCounterResetWatchdog      = "CNT_RST_WDG";
    static constexpr SettingsNS::tKey mKeyCounterResetPanic         = "CNT_RST_PANIC";
    static constexpr SettingsNS::tKey mKeyCounterResetBrownout      = "CNT_RST_BRNOUT";


    /* Keys for display settings */
    static constexpr SettingsNS::tKey  mKeyDisplayClockMode             = "DSP_CLK_MODE";
    static constexpr SettingsNS::tKey  mKeyDisplayClockItIs             = "DSP_CLK_ITIS";
    static constexpr SettingsNS::tKey  mKeyDisplayClockSingleMins       = "DSP_CLK_SMINS";
    static constexpr SettingsNS::tKey  mKeyDisplayColorTime             = "DSP_COLOR_TIME";
    static constexpr SettingsNS::tKey  mKeyDisplayColorBkgd             = "DSP_COLOR_BKGD";

    static constexpr SettingsNS::tKey  mKeyDisplayLedBrightness         = "DSP_LED_BRIGHT";

    static constexpr SettingsNS::tKey  mKeyDisplayUseNightMode          = "DSP_NITE_MODE";
    static constexpr SettingsNS::tKey  mKeyDisplayBrightnessNightMode   = "DSP_NITE_BRIGHT";
    static constexpr SettingsNS::tKey  mKeyDisplayNightModeStartTime    = "DSP_NITE_START";
    static constexpr SettingsNS::tKey  mKeyDisplayNightModeEndTime      = "DSP_NITE_END";


    /* Default values for display settings */
    static constexpr uint8_t  mDefaultDisplayClockMode                  = 1;            // Rhein-Ruhr
    static constexpr bool     mDefaultDisplayClockItIs                  = true;
    static constexpr bool     mDefaultDisplayClockSingleMins            = true;
    static constexpr uint32_t mDefaultDisplayColorTime                  = 0x00FF00;     // Green
    static constexpr uint32_t mDefaultDisplayColorBkgd                  = 0x000000;     // Black

    static constexpr bool     mDefaultDisplayUseNightMode               = true;

    /*
    * Brightness values in percentage (0-100)
    */
    static constexpr uint8_t  mDefaultDisplayLedBrightness              = 100;
    static constexpr uint8_t  mDefaultDisplayBrightnessNightMode        = 20;

    /*
    * Dword (4 bytes) for savinig a time in format hh:mm
    *  00000000 0000000s ssssmmmm mm000000
    *                        ---- --            Minutes  : range 0-59; 6 bits, mask 0x3F, offset 06
    *                  - ----                   Hours    : range 0-23; 5 bits, mask 0x1F, offset 12
    */
    static constexpr uint32_t mDefaultDisplayNightModeStartTime         = (21 & 0x3F) << 12 | (30 & 0x3F) << 06;    // 21:30
    static constexpr uint32_t mDefaultDisplayNightModeEndTime           = (06 & 0x3F) << 12 | (30 & 0x3F) << 06;    // 06:30

    static constexpr uint8_t mcClockModeItemsCount = 2;
    static constexpr const char* mcClockModeItems[mcClockModeItemsCount] = {
        "Wessi",
        "Rhein-Ruhr"
    };

}   /* end of namespace ConfigNS */