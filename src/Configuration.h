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
    static constexpr SettingsNS::tKey mKeyCounterResetPowerOn   = "CNT_RST_PWRON";
    static constexpr SettingsNS::tKey mKeyCounterResetSoftware  = "CNT_RST_SW";
    static constexpr SettingsNS::tKey mKeyCounterResetWatchdog  = "CNT_RST_WDG";
    static constexpr SettingsNS::tKey mKeyCounterResetPanic     = "CNT_RST_PANIC";
    static constexpr SettingsNS::tKey mKeyCounterResetBrownout  = "CNT_RST_BRNOUT";

}   /* end of namespace ConfigNS */