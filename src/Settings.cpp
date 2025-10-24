/*
 * Settings.cpp
 *
 *  Created on: 16.10.2025
 *      Author: hocki
 */
#include <Arduino.h>
#include <type_traits>

#include "Settings.hpp"


namespace SettingsNS
{
/**
 * @brief Constructor
 */
Settings::Settings()
{
    // do nothing
}

/**
 * @brief Destructor
 */
Settings::~Settings()
{
    // do nothing
}

/**
 * @brief Clear all settings in ESP32 Preferences storage.
 */
void Settings::Clear(void)
{
    /* Open preferences in read-write mode */
    if (mPrefs.begin(mcPrefsParamNamespace, false))
    {
        /* Clear all keys */
        mPrefs.clear();
        mPrefs.end();
    }
}

/**
 * @brief Checks if a key exists in ESP32 Preferences storage.
 *
 * @param arKey The name of the key to check.
 * @return true if the key exists, false otherwise.
 */
bool Settings::HasKey(tKey arKey)
{
    bool wRetValue = false;

    /* Open preferences in read-only mode */
    if (mPrefs.begin(mcPrefsParamNamespace, true))
    {
        /* Check if the key exists */
        wRetValue = mPrefs.isKey(arKey);
        /* Close the Preferences */
        mPrefs.end();
    }
    return wRetValue;
}

/**
 * @brief Deletes a key-value pair from ESP32 Preferences storage.
 *
 * @param arKey The name of the key to remove.
 * @return true if the key was successfully removed, false otherwise.
 */
bool Settings::RemoveKey(tKey arKey)
{
    bool wRetValue = false;

    /* Open preferences in read-write mode */
    if (mPrefs.begin(mcPrefsParamNamespace, false))
    {
        /* Remove the key */
        wRetValue = mPrefs.remove(arKey);
        /* Close the Preferences */
        mPrefs.end();
    }
    return wRetValue;
}

/**
 * @brief Retrieves a byte array from ESP32 Preferences storage.
 *
 * @param arKey The name of the key.
 * @param apData Pointer to the buffer to store the retrieved byte array.
 * @param aDataSize Size of the buffer in bytes.
 * @return true if the data was successfully read, false otherwise.
 */
bool Settings::GetBytes(tKey arKey, uint8_t* apData, const size_t aDataSize)
{
    size_t wRetSize = 0;

    /* Open preferences in read-only mode */
    if (mPrefs.begin(mcPrefsParamNamespace, true))
    {
        wRetSize = mPrefs.getBytes(arKey, apData, aDataSize);

        mPrefs.end();
    }

    return (wRetSize == aDataSize);
}



/**
 * @brief Stores a byte array in ESP32 Preferences storage.
 *
 * @param arKey The name of the key.
 * @param apData Pointer to the byte array to store.
 * @param aDataSize Size of the byte array in bytes.
 * @return true if the data was successfully written, false otherwise.
 */
bool Settings::SetBytes(tKey arKey, const uint8_t* apData, const size_t aDataSize)
{
    size_t wRetSize = 0;

    /* Open preferences in read-write mode */
    if (mPrefs.begin(mcPrefsParamNamespace, false))
    {
        wRetSize = mPrefs.putBytes(arKey, apData, aDataSize);

        mPrefs.end();
    }

    return (wRetSize == aDataSize);
}

/**
 * @brief Increases or sets a counter value.
 *
 * @details
 * This function increments the value of a counter stored in the "counters" preferences namespace.
 * If aNewValue is provided and not zero, the counter is set to this value; otherwise, the counter
 * is incremented by one. Using a dedicated namespace keeps counter values separate from other settings,
 * improving organization and access.
 *
 * @param arKey The name of the counter key.
 * @param aNewValue The new value to set for the counter (if not zero), otherwise the counter is incremented.
 */
void Settings::IncreaseCounter(tKey arKey, const uint32_t aNewValue)
{
    /* Get current counter value */
    uint32_t wCounter = GetCounter(arKey, 0);

    /* Open preferences in read-write mode */
    if (mPrefs.begin(mcPrefsCounterNamespace, false))
    {
        mPrefs.putUInt(arKey, (aNewValue != 0) ? aNewValue : (wCounter + 1));
        mPrefs.end();
    }
}

/**
 * @brief Retrieves a counter value.
 *
 * @details
 * This function reads the value of a counter stored in the "counters" preferences namespace.
 * If the key does not exist, it returns the provided default value. The use of a dedicated
 * namespace keeps counter values separate from other settings for better organization.
 *
 * @param arKey The name of the counter key.
 * @param aDefaultValue The value to return if the counter key does not exist.
 * @return The counter value associated with the key, or aDefaultValue if not found.
 */
uint32_t Settings::GetCounter(tKey arKey, const uint32_t aDefaultValue)
{
    uint32_t wCounter = aDefaultValue;

    /* Open preferences in read-only mode */
    if (mPrefs.begin(mcPrefsCounterNamespace, true))
    {
        wCounter = mPrefs.getUInt(arKey, 0);
        mPrefs.end();
    }

    return wCounter;
}

}   /* end of namespace SettingsNS */