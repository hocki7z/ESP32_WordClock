/*
 * Settings.hpp
 *
 *  Created on: 16.10.2025
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>
#include <variant>

#include <Preferences.h>

/**
 * @brief Namespace for settings related definitions
 */
namespace SettingsNS
{
    /**
     * @typedef tKey
     * @brief Alias for settings key type.
     *
     * @details
     * The tKey type is defined as a constant character pointer and is used to represent
     * keys for accessing settings in persistent storage. It provides a convenient and
     * readable way to specify keys when storing or retrieving values from the Preferences API.
     */
    typedef const char* tKey;

    enum class SettingType {
        BOOL,
        UINT8,
        UINT16,
        UINT32,
        INT8,
        INT16,
        INT32,
        FLOAT,
        DOUBLE,
        STRING
    };

    struct SettingItem
    {
        SettingsNS::tKey mKey;
        SettingType     mType;
        std::variant<bool, uint8_t, uint16_t, uint32_t, int8_t, int16_t, int32_t, float, double, String> mDefaultValue;
    };


    /**
     * @brief Settings class for managing persistent configuration storage.
     *
     * @details
     * The Settings class provides an interface for storing, retrieving, and managing configuration
     * values in persistent storage using the ESP32 Preferences API. It supports various data types
     * through templated GetValue and SetValue methods, as well as byte arrays and counters.
     * The class also offers methods to clear all settings, check for the existence of keys,
     * remove keys, and increment counters. All operations are performed within the "prefs" namespace.
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
        uint32_t GetCounter(tKey arKey, const uint32_t aDefaultValue = 0);

    private:

        /** @brief Namespace name for parameters storage in Preferences */
        static constexpr const char* mcPrefsParamNamespace = "params";

        /** @brief Namespace name for counter storage in Preferences */
        static constexpr const char* mcPrefsCounterNamespace = "counters";

        Preferences mPrefs;
    };

}   /* end of namespace SettingsNS */

/* Include the template implementation file */
#include "Settings.tpp"

/* Declare the object as extern for global access  */
extern SettingsNS::Settings Settings;
