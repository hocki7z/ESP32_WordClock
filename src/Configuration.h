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
    static constexpr uint32_t      mWifiManagerTaskStackSize = mDefaultTaskStackSize + 512;
    static constexpr const char*   mWifiManagerTaskName      = "WifiManagerTask";

    /* Web site task configuration */
    static constexpr tTaskPriority mWebSiteTaskPriority      = mDefaultTaskPriority;
    static constexpr uint32_t      mWebSiteTaskStackSize     = mDefaultTaskStackSize;
    static constexpr const char*   mWebSiteTaskName          = "WebSiteTask";


    /**
     * WiFi configurations
     */
    /** @brief WiFi access point SSID and password */
    static constexpr const char*   mWiFiApSSID               = "ESP32_WORDCLOCK";
    static constexpr const char*   mWiFiApPASS               = "12345678";         // Leave empty for open network
    /** @brief DNS server port */
    static constexpr uint8_t       mDnsPort                  =  53;


    /**
     * Settings configurations
     */
    static constexpr uint8_t      mParamsConfig                     = 0x00;
    static constexpr uint8_t      mCountersConfig                   = 0x01;

    static constexpr uint8_t      mApplicationGroup                 = MessageNS::tAddress::APPLICATION_MANAGER;
    static constexpr uint8_t      mDisplayGroup                     = MessageNS::tAddress::DISPLAY_MANAGER;
    static constexpr uint8_t      mTimeManagerGroup                 = MessageNS::tAddress::TIME_MANAGER;

    /* Keys for reset counters */
    static const SettingsNS::tKey mKeyCounterResetPowerOn           = SettingsNS::tKey(mCountersConfig, mApplicationGroup, 0x00);
    static const SettingsNS::tKey mKeyCounterResetSoftware          = SettingsNS::tKey(mCountersConfig, mApplicationGroup, 0x01);
    static const SettingsNS::tKey mKeyCounterResetWatchdog          = SettingsNS::tKey(mCountersConfig, mApplicationGroup, 0x02);
    static const SettingsNS::tKey mKeyCounterResetPanic             = SettingsNS::tKey(mCountersConfig, mApplicationGroup, 0x03);
    static const SettingsNS::tKey mKeyCounterResetBrownout          = SettingsNS::tKey(mCountersConfig, mApplicationGroup, 0x04);


    /* Keys for display settings */
    static const SettingsNS::tKey  mKeyDisplayClockMode             = SettingsNS::tKey(mParamsConfig, mDisplayGroup, 0x00);
    static const SettingsNS::tKey  mKeyDisplayClockItIs             = SettingsNS::tKey(mParamsConfig, mDisplayGroup, 0x01);
    static const SettingsNS::tKey  mKeyDisplayClockSingleMins       = SettingsNS::tKey(mParamsConfig, mDisplayGroup, 0x02);
    static const SettingsNS::tKey  mKeyDisplayColorTime             = SettingsNS::tKey(mParamsConfig, mDisplayGroup, 0x10);
    static const SettingsNS::tKey  mKeyDisplayColorBkgd             = SettingsNS::tKey(mParamsConfig, mDisplayGroup, 0x11);

    static const SettingsNS::tKey  mKeyDisplayLedBrightness         = SettingsNS::tKey(mParamsConfig, mDisplayGroup, 0x20);
    static const SettingsNS::tKey  mKeyDisplayUseNightMode          = SettingsNS::tKey(mParamsConfig, mDisplayGroup, 0x21);
    static const SettingsNS::tKey  mKeyDisplayBrightnessNightMode   = SettingsNS::tKey(mParamsConfig, mDisplayGroup, 0x22);
    static const SettingsNS::tKey  mKeyDisplayNightModeStartTime    = SettingsNS::tKey(mParamsConfig, mDisplayGroup, 0x23);
    static const SettingsNS::tKey  mKeyDisplayNightModeEndTime      = SettingsNS::tKey(mParamsConfig, mDisplayGroup, 0x24);

    static const SettingsNS::tKey  mKeyNtpServer                    = SettingsNS::tKey(mParamsConfig, mTimeManagerGroup, 0x00);
    static const SettingsNS::tKey  mKeyNtpSyncPeriod                = SettingsNS::tKey(mParamsConfig, mTimeManagerGroup, 0x01);
    static const SettingsNS::tKey  mKeyNtpSyncTimeout               = SettingsNS::tKey(mParamsConfig, mTimeManagerGroup, 0x02);

    static const SettingsNS::tKey  mKeyTimeZone                     = SettingsNS::tKey(mParamsConfig, mTimeManagerGroup, 0x10);


    /* Default values for display settings */
    static constexpr uint8_t  mDefaultDisplayClockMode              = 1;            // Rhein-Ruhr
    static constexpr bool     mDefaultDisplayClockItIs              = true;
    static constexpr bool     mDefaultDisplayClockSingleMins        = true;
    static constexpr uint32_t mDefaultDisplayColorTime              = 0x00FF00;     // Green
    static constexpr uint32_t mDefaultDisplayColorBkgd              = 0x000000;     // Black

    static constexpr bool     mDefaultDisplayUseNightMode           = true;

    /*
     * Brightness values in percentage (0-100)
     */
    static constexpr uint8_t  mDefaultDisplayLedBrightness          = 100;
    static constexpr uint8_t  mDefaultDisplayBrightnessNightMode    = 20;

    /*
    * Dword (4 bytes) for savinig a time in format hh:mm
    *  00000000 0000000s ssssmmmm mm000000
    *                        ---- --            Minutes  : range 0-59; 6 bits, mask 0x3F, offset 06
    *                  - ----                   Hours    : range 0-23; 5 bits, mask 0x1F, offset 12
    */
    static constexpr uint32_t mDefaultDisplayNightModeStartTime     = (21 & 0x3F) << 12 | (30 & 0x3F) << 06;    // 21:30
    static constexpr uint32_t mDefaultDisplayNightModeEndTime       = (06 & 0x3F) << 12 | (30 & 0x3F) << 06;    // 06:30

    /* Default values for datetime settings */
    static constexpr uint8_t mDefaultNtpServer                      = 0;            // "pool.ntp.org"
    static constexpr uint8_t mDefaultTimeZone                       = 6;            // CET timezone
    static constexpr uint32_t mDefaultNtpSyncPeriod                 = 600;          // 10 minutes
    static constexpr uint32_t mDefaultNtpSyncTimeout                = 5000;         // 5 seconds




    static constexpr uint8_t mcClockModeItemsCount = 2;
    static constexpr const char* mcClockModeItems[mcClockModeItemsCount] = {
        "Wessi",
        "Rhein-Ruhr"
    };

    static constexpr uint8_t mcNtpServerItemsCount = 10;
    static constexpr const char* mcNtpServerItems[mcNtpServerItemsCount] = {
        "pool.ntp.org",                     // NTP Pool Project servers
        "europe.pool.ntp.org",
        "north-america.pool.ntp.org",
        "asia.pool.ntp.org",
        "ru.pool.ntp.org",
        "time.google.com",                  // Google NTP servers
        "time.android.com",
        "time.windows.com",                 // Microsoft NTP server
        "time.aws.com",                     // Amazon NTP servers
        "amazon.pool.ntp.org"
    };


    /*
     * The Most Popular Time Zone in the World
     *     https://www.worldtimeserver.com/learn/the-most-popular-time-zone-in-the-world/
     */
    static constexpr uint8_t mcTimezoneItemsCount = 16;
    static constexpr const char* mcTimezoneNames[mcTimezoneItemsCount] = {
        /* AEST */  "Australia Eastern Standard Time",
        /* ACST */  "Australia Central Standard Time",
        /* AFT  */  "Afghanistan Time",
        /* AKST */  "Alaska Standard Time (DTS)",
        /* AST  */  "Atlantic Standard Time",
        /* CAT  */  "Central Africa Time",
        /* CET  */  "Central European Time (DTS)",
        /* CST  */  "Central Standard Time (DTS)",
        /* EAT  */  "East Africa Time (DTS)",
        /* EET  */  "Eastern European Time (DTS)",
        /* EST  */  "Eastern Standard Time (DTS)",
        /* MSK  */  "Moscow Standard Time",
        /* MST  */  "Mountain Standard Time (DTS)",
        /* PST  */  "Pacific Standard Time (DTS)",
        /* WAT  */  "West Africa Time",
        /* WET  */  "Western European Time (DTS)",
    };

    static constexpr const char* mcTimezones[mcTimezoneItemsCount] = {
        /* AEST */  "AEST-10",
        /* ACST */  "ACST-9:30",
        /* AFT  */  "AFT-4:30",
        /* AKST */  "AKST9AKDT,M3.2.0,M11.1.0",
        /* AST  */  "AST4",
        /* CAT  */  "CAT-2",
        /* CET  */  "CET-1CEST,M3.5.0,M10.5.0/3",
        /* CST  */  "CST6CDT,M3.2.0,M11.1.0",
        /* EAT  */  "EET-2EEST,M3.5.0/3,M10.5.0/4",
        /* EET  */  "EET-2EEST,M3.5.0/3,M10.5.0/4",
        /* EST  */  "EST5EDT,M3.2.0,M11.1.0",
        /* MSK  */  "MSK-3",
        /* MST  */  "MST7MDT,M3.2.0,M11.1.0",
        /* PST  */  "PST8PDT,M3.2.0,M11.1.0",
        /* WAT  */  "WAT-1",
        /* WET  */  "WET0WEST,M3.5.0/1,M10.5.0",
    };
}   /* end of namespace ConfigNS */