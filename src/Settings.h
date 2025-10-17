/*
 * Settings.h
 *
 *  Created on: 16.10.2025
 *      Author: hocki
 */

#ifndef SRC_SETTINGS_H_
#define SRC_SETTINGS_H_

#include <Arduino.h>
#include <Preferences.h>


/**
 * @brief Namespace for settings related definitions
 */
namespace SettingsNS
{
    class Settings
    {
    public:
        Settings() {}
        virtual ~Settings() {}

        /**
         * @brief Clear all settings in ESP32 Preferences storage.
         */
        void Clear(void)
        {
            if (mPrefs.begin(mcPrefsName, false))  // false - read-write mode
            {
                /* Clear all keys */
                mPrefs.clear();
                mPrefs.end();
            }
        }

        /**
         * @brief Get a property value as a specific type from ESP32 Preferences storage.
         *
         * @details Opens the preferences in read-only mode and retrieves the value for the given key using the appropriate method for the type.
         * Supported types are: bool, uint8_t, uint16_t, uint32_t, int8_t, int16_t, int32_t, float, double, String.
         * If the key does not exist, the provided default value is returned.
         * If an unsupported type is requested, a compile-time error is generated.
         *
         * @tparam T The type of the property value to retrieve (e.g., int, bool, String).
         * @param arKey The name of the property.
         * @param aDefaultValue The default value to return if the property does not exist.
         * @return The property value as type T, or the default value if not found.
         *
         * Exalmpes:
         *      int myInt = Settings.GetValue<int>("myIntKey", 42);
         *      bool myBool = Settings.GetValue<bool>("myBoolKey", false);
         *      String myString = Settings.GetValue<String>("myStringKey", "default");
         *
         *   Unsupported type (will cause compile-time error):
         *      std::vector<int> myVector = Settings.GetValue<std::vector<int>>("myVectorKey", {});
         */
        template<typename T> T GetValue(const String& arKey, T aDefaultValue)
        {
            T wRetValue = aDefaultValue;

            if ((HasKey(arKey) == true) &&           // check if key exists
                (mPrefs.begin(mcPrefsName, true)))  // true - read only mode
            {
                // Use type traits or if constexpr (C++17+) for type-specific logic
                if constexpr (std::is_same<T, bool>::value) {
                    wRetValue = mPrefs.getBool(arKey.c_str(), aDefaultValue);
                } else if constexpr (std::is_same<T, uint8_t>::value) {
                    wRetValue = mPrefs.getUChar(arKey.c_str(), aDefaultValue);
                } else if constexpr (std::is_same<T, uint16_t>::value) {
                    wRetValue = mPrefs.getUShort(arKey.c_str(), aDefaultValue);
                } else if constexpr (std::is_same<T, uint32_t>::value) {
                    wRetValue = mPrefs.getUInt(arKey.c_str(), aDefaultValue);
                } else if constexpr (std::is_same<T, int8_t>::value) {
                    wRetValue = mPrefs.getChar(arKey.c_str(), aDefaultValue);
                } else if constexpr (std::is_same<T, int16_t>::value) {
                    wRetValue = mPrefs.getShort(arKey.c_str(), aDefaultValue);
                } else if constexpr (std::is_same<T, int32_t>::value) {
                    wRetValue = mPrefs.getInt(arKey.c_str(), aDefaultValue);
                } else if constexpr (std::is_same<T, float>::value) {
                    wRetValue = mPrefs.getFloat(arKey.c_str(), aDefaultValue);
                } else if constexpr (std::is_same<T, double>::value) {
                    wRetValue = mPrefs.getDouble(arKey.c_str(), aDefaultValue);
                } else if constexpr (std::is_same<T, String>::value) {
                    wRetValue = mPrefs.getString(arKey.c_str(), aDefaultValue);
                }
                else
                {
                    /* Type is not directly supported, generate  exception */
                    static_assert(sizeof(T) == 0, "Unsupported type for GetValue function");
                }

                mPrefs.end();
            }

            return wRetValue;
        }

        /**
         * @brief Retrieves a byte array from ESP32 Preferences storage.
         * @param arKey The name of the key.
         * @param apData Pointer to the buffer to store the retrieved byte array.
         * @param aDataSize Size of the buffer in bytes.
         * @return true if the data was successfully read, false otherwise.
         */
        bool GetBytes(const String& arKey, uint8_t* apData, size_t aDataSize)
        {
            size_t wRetSize = 0;

            if (mPrefs.begin(mcPrefsName, true))  // true - read only mode
            {
                wRetSize = mPrefs.getBytes(arKey.c_str(), apData, aDataSize);

                mPrefs.end();
            }

            return (wRetSize == aDataSize);
        }

        /**
         * @brief Sets a property value in ESP32 Preferences storage.
         *
         * @details Stores the value for the given key in the preferences using the appropriate method for the type.
         * Only supported types (bool, uint8_t, uint16_t, uint32_t, int8_t, int16_t, int32_t, float, double, String) are accepted.
         * Unsupported types will cause a compile-time error.
         *
         * @tparam T The type of the property value to store (e.g., int, bool, String).
         * @param arKey The name of the property.
         * @param aValue The value to store.
         * @return true if the value was successfully written, false otherwise.
         *
         * Examples:
         *      bool success = Settings.SetValue<int>("myIntKey", 42);
         *      bool success = Settings.SetValue<bool>("myBoolKey", true);
         *      bool success = Settings.SetValue<String>("myStringKey", "Hello");
         *
         *    Unsupported type (will cause compile-time error):
         *     bool success = settings.SetValue<std::vector<int>>("myVectorKey", {1,2,3});
         */
        template<typename T> bool SetValue(const String& arKey, T aValue)
        {
            size_t wRetSize = 0;

            if (mPrefs.begin(mcPrefsName, false))  // false - read-write mode
            {
                if constexpr (std::is_same<T, bool>::value) {
                    wRetSize = mPrefs.putBool(arKey.c_str(), aValue);
                } else if constexpr (std::is_same<T, uint8_t>::value) {
                    wRetSize = mPrefs.putUChar(arKey.c_str(), aValue);
                } else if constexpr (std::is_same<T, uint16_t>::value) {
                    wRetSize = mPrefs.putUShort(arKey.c_str(), aValue);
                } else if constexpr (std::is_same<T, uint32_t>::value) {
                    wRetSize = mPrefs.putUInt(arKey.c_str(), aValue);
                } else if constexpr (std::is_same<T, int8_t>::value) {
                    wRetSize = mPrefs.putChar(arKey.c_str(), aValue);
                } else if constexpr (std::is_same<T, int16_t>::value) {
                    wRetSize = mPrefs.putShort(arKey.c_str(), aValue);
                } else if constexpr (std::is_same<T, int32_t>::value) {
                    wRetSize = mPrefs.putInt(arKey.c_str(), aValue);
                } else if constexpr (std::is_same<T, float>::value) {
                    wRetSize = mPrefs.putFloat(arKey.c_str(), aValue);
                } else if constexpr (std::is_same<T, double>::value) {
                    wRetSize = mPrefs.putDouble(arKey.c_str(), aValue);
                } else if constexpr (std::is_same<T, String>::value) {
                    wRetSize = mPrefs.putString(arKey.c_str(), aValue);
                } else {
                    /* Type is not directly supported, generate  exception */
                    static_assert(sizeof(T) == 0, "Unsupported type for SetValue function");
                }

                mPrefs.end();
            }

            return (wRetSize == sizeof(T));
        }

        /**
         * @brief Stores a byte array in ESP32 Preferences storage.
         * @param arKey The name of the key.
         * @param apData Pointer to the byte array to store.
         * @param aDataSize Size of the byte array in bytes.
         * @return true if the data was successfully written, false otherwise.
         */
        bool SetBytes(const String& arKey, const uint8_t* apData, size_t aDataSize)
        {
            size_t wRetSize = 0;

            if (mPrefs.begin(mcPrefsName, false))  // false - read-write mode
            {
                wRetSize = mPrefs.putBytes(arKey.c_str(), apData, aDataSize);

                mPrefs.end();
            }

            return (wRetSize == aDataSize);
        }

        /**
         * @brief Deletes a key-value pair from ESP32 Preferences storage.
         * @param arKey The name of the key to remove.
         * @return true if the key was successfully removed, false otherwise.
         */
        bool RemoveKey(const String& arKey)
        {
            bool wRetValue = false;
            /* Start a namespace "prefs" */
            if (mPrefs.begin(mcPrefsName, false))  // false - read-write mode
            {
                /* Remove the key  */
                wRetValue = mPrefs.remove(arKey.c_str());
                /* Close the Preferences */
                mPrefs.end();
            }
            return wRetValue;
        }

        /**
         * @brief Checks if a key exists in ESP32 Preferences storage.
         * @param arKey The name of the key to check.
         * @return true if the key exists, false otherwise.
         */
        bool HasKey(const String& arKey)
        {
            bool wRetValue = false;
            /* Start a namespace "prefs" */
            if (mPrefs.begin(mcPrefsName, true))  // true - read only mode
            {
                /* Check if the key exists */
                wRetValue = mPrefs.isKey(arKey.c_str());
                /* Close the Preferences */
                mPrefs.end();
            }
            return wRetValue;
        }

        /**
         * @brief Increases a counter stored in preferences by 1, or sets it to a new value if provided.
         * @param arKey The name of the counter key.
         * @param aNewValue Optional new value to set the counter to. If 0, the counter is incremented by 1.
         */
        void IncreaseCounter(const String& arKey, uint32_t aNewValue = 0)
        {
            uint32_t wCounter = GetValue<uint32_t>(arKey, 0);

            SetValue<uint32_t>(arKey, (aNewValue != 0) ? aNewValue : (wCounter + 1));
        }

        /**
         * @brief Retrieves the current value of a counter stored in preferences.
         * @param arKey The name of the counter key.
         * @return The current counter value, or 0 if the key does not exist.
         */
        uint32_t GetCounter(const String& arKey)
        {
            return GetValue<uint32_t>(arKey, 0);
        }

    private:

        static constexpr const char* mcPrefsName = "prefs";
        /* Instance of preferences */
        Preferences mPrefs;
    };
}   /* end of namespace SettingsNS */


/* Instance of Settings class */
SettingsNS::Settings Settings;

#endif /* SRC_SETTINGS_H_ */
