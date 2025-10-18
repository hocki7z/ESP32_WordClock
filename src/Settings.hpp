/*
 * Settings.hpp
 *
 *  Created on: 16.10.2025
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>
#include <Preferences.h>

/**
 * @brief Namespace for settings related definitions
 */
namespace SettingsNS
{
    /* Type definition for settings keys */
    typedef const char* tKey;


    /* Keys for reset counters */
    static constexpr tKey mKeyCounterResetPowerOn   = "CNT_RST_PWRON";
    static constexpr tKey mKeyCounterResetSoftware  = "CNT_RST_SW";
    static constexpr tKey mKeyCounterResetWatchdog  = "CNT_RST_WDG";
    static constexpr tKey mKeyCounterResetPanic     = "CNT_RST_PANIC";
    static constexpr tKey mKeyCounterResetBrownout  = "CNT_RST_BRNOUT";

    /**
     * @brief Class for managing persistent settings using Preferences
     */
    class Settings
    {
    public:
        Settings();
        virtual ~Settings();

        void Clear(void);

        bool HasKey(tKey arKey);
        bool RemoveKey(tKey arKey);

        template<typename T>
        T GetValue(tKey arKey, const T aDefaultValue);

        template<typename T>
        bool SetValue(tKey arKey, const T aValue);

        bool GetBytes(tKey arKey, uint8_t* apData, const size_t aDataSize);
        bool SetBytes(tKey arKey, const uint8_t* apData, const size_t aDataSize);

        void IncreaseCounter(tKey arKey, const uint32_t aNewValue = 0);
        uint32_t GetCounter(tKey arKey);

    private:
        static constexpr const char* mcPrefsName = "prefs";
        Preferences mPrefs;
    };

}   /* end of namespace SettingsNS */

/* Include the template implementation file */
#include "Settings.tpp"

/* Declare the object as extern for global access  */
extern SettingsNS::Settings Settings;
