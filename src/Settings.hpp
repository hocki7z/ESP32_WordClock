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
    /**
     * @brief Key definition for settings storage
     *
     * @details
     * The tKey type is defined as a uint32_t to provide better type safety when
     * working with settings keys. The key is structured in a way that allows
     * grouping and categorization of settings using a hierarchical approach.
     *
     * The key is divided into four bytes as follows: 0xAABBCCCC
     *       Byte 3 | Byte 2 | Byte 1 | Byte 0
     *       -------|--------|--------|--------
     *        AA    |   BB   |   CC   |   CC
     *
     *       AA:   Region (Major group)
     *       BB:     Group
     *       CCCC:     Key id
     */
    union tKey
    {
        uint32_t mRaw;              // Full 32-bit key raw value
        struct
        {
            uint16_t mId;           // Byte 0: Key id in range: 0x0000 .. 0xFFFF
            uint8_t  mGroup;        // Byte 2: Group in rage: 0x00 .. 0xFF
            uint8_t  mRegion;       // Byte 3: Region (Major group) in range: 0x00 .. 0xFF
        } mParts;

        /**
         * @brief Default constructor for tKey.
         *
         * @details
         * Initializes the key to zero. This creates a key with all fields set to 0,
         * representing an uninitialized or default key value.
         */
        tKey() : mRaw(0)
        {
            // do nothing
        }

        /**
         * @brief Constructor for tKey from a raw 32-bit value.
         *
         * @details
         * Initializes the key using a raw uint32_t value. This allows direct assignment
         * of a key from a packed 32-bit representation.
         *
         * @param aRaw Raw 32-bit value to initialize the key.
         */
        tKey(uint32_t aRaw) : mRaw(aRaw)
        {
            // do nothing
        }

        /**
         * @brief Constructor for tKey from region, group, and key parts.
         *
         * @details
         * Initializes the key using individual region, group, and ID values.
         * This constructor sets each part of the key structure, allowing for
         * easy creation of hierarchical keys.
         *
         * @param aRegion Region (major group) value.
         * @param aGroup  Group (minor group) value.
         * @param aKey    Key id value.
         */
        tKey(uint8_t aRegion, uint8_t aGroup, uint16_t aId)
        {
            mParts.mRegion = aRegion;
            mParts.mGroup  = aGroup;
            mParts.mId     = aId;
        }

        /**
         * @brief Equality operator for tKey.
         *
         * @details
         * Compares two tKey objects for equality by checking if their raw values match.
         *
         * @param other The tKey object to compare with.
         * @return True if both keys are equal, false otherwise.
         */
        bool operator==(const tKey& other) const
        {
            return mRaw == other.mRaw;
        }

        /**
         * @brief Inequality operator for tKey.
         *
         * @details
         * Compares two tKey objects for inequality by checking if their raw values differ.
         *
         * @param other The tKey object to compare with.
         * @return True if the keys are not equal, false otherwise.
         */
        bool operator!=(const tKey& other) const
        {
            return mRaw != other.mRaw;
        }
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

        bool HasKey(const tKey& arKey);
        bool RemoveKey(const tKey& arKey);

        template<typename T>
        T GetValue(const tKey& arKey, const T aDefaultValue);

        template<typename T>
        bool SetValue(const tKey& arKey, const T aValue);

        bool GetBytes(const tKey& arKey, uint8_t* apData, const size_t aDataSize);
        bool SetBytes(const tKey& arKey, const uint8_t* apData, const size_t aDataSize);

        bool IncreaseCounter(const tKey& arKey, const uint32_t aNewValue = 0);
        uint32_t GetCounter(const tKey& arKey, const uint32_t aDefaultValue = 0);

    private:

        /** @brief Namespace name for parameters storage in Preferences */
        static constexpr const char* mcPrefsParamNamespace = "params";

        /** @brief Namespace name for counter storage in Preferences */
        static constexpr const char* mcPrefsCounterNamespace = "counters";

        Preferences mPrefs;

        /**
         * @brief Converts a tKey to its string representation.
         *
         * @param arKey The tKey instance to convert.
         * @return Pointer to a static char buffer containing the formatted key string.
         */
        const char* GetString(const tKey& arKey) const
        {
            static char wKeyStr[9]; // 2 + 2 + 4 + null terminator

            snprintf(wKeyStr, sizeof(wKeyStr), "%02X%02X%04X",
                    arKey.mParts.mRegion,
                    arKey.mParts.mGroup,
                    arKey.mParts.mId);

            return wKeyStr;
        }
    };

}   /* end of namespace SettingsNS */

/* Include the template implementation file */
#include "Settings.tpp"

/* Declare the object as extern for global access  */
extern SettingsNS::Settings Settings;
