/*
 * Settings.tpp
 *
 *  Created on: 16.10.2025
 *      Author: hocki
 */

namespace SettingsNS
{

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
 *      int myInt = Settings.GetValue<int>(tKey(1234), 42);
 *      bool myBool = Settings.GetValue<bool>(tKey(1234), false);
 *      String myString = Settings.GetValue<String>(tKey(1234), "default");
 *
 *   Unsupported type (will cause compile-time error):
 *      std::vector<int> myVector = Settings.GetValue<std::vector<int>>(tKey(1234), {});
 */
template<typename T>
T Settings::GetValue(const tKey& arKey, const T aDefaultValue)
{
    T wRetValue = aDefaultValue;

    /* Check if key exists and open preferences in read-only mode */
    if ((HasKey(arKey) == true) &&
        (mPrefs.begin(mcPrefsParamNamespace, true)))
    {
        /* Get key string representation */
        const char* wKeyStr = GetString(arKey);

        if constexpr (std::is_same<T, bool>::value) {
            wRetValue = mPrefs.getBool(wKeyStr, aDefaultValue);
        } else if constexpr (std::is_same<T, uint8_t>::value)   {
            wRetValue = mPrefs.getUChar(wKeyStr, aDefaultValue);
        } else if constexpr (std::is_same<T, uint16_t>::value)  {
            wRetValue = mPrefs.getUShort(wKeyStr, aDefaultValue);
        } else if constexpr (std::is_same<T, uint32_t>::value)  {
            wRetValue = mPrefs.getUInt(wKeyStr, aDefaultValue);
        } else if constexpr (std::is_same<T, int8_t>::value)    {
            wRetValue = mPrefs.getChar(wKeyStr, aDefaultValue);
        } else if constexpr (std::is_same<T, int16_t>::value)   {
            wRetValue = mPrefs.getShort(wKeyStr, aDefaultValue);
        } else if constexpr (std::is_same<T, int32_t>::value)   {
            wRetValue = mPrefs.getInt(wKeyStr, aDefaultValue);
        } else if constexpr (std::is_same<T, float>::value)     {
            wRetValue = mPrefs.getFloat(wKeyStr, aDefaultValue);
        } else if constexpr (std::is_same<T, double>::value)    {
            wRetValue = mPrefs.getDouble(wKeyStr, aDefaultValue);
        } else if constexpr (std::is_same<T, String>::value)    {
            wRetValue = mPrefs.getString(wKeyStr, aDefaultValue);
        }
        else
        {
            /* Type is not directly supported, generate  exception */
            static_assert(sizeof(T) == 0, "Unsupported type for GetValue() function");
        }

        mPrefs.end();
    }

    return wRetValue;
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
 *      bool success = Settings.SetValue<int>(tKey(1234), 42);
 *      bool success = Settings.SetValue<bool>(tKey(1234), true);
 *      bool success = Settings.SetValue<String>(tKey(1234), "Hello");
 *
 *    Unsupported type (will cause compile-time error):
 *      bool success = settings.SetValue<std::vector<int>>(tKey(1234), {1,2,3});
 */
template<typename T>
bool Settings::SetValue(const tKey& arKey, const T aValue)
{
    size_t wRetSize = 0;

    /* Open preferences in read-write mode */
    if (mPrefs.begin(mcPrefsParamNamespace, false))
    {
        /* Get key string representation */
        const char* wKeyStr = GetString(arKey);

        if constexpr (std::is_same<T, bool>::value) {
            wRetSize = mPrefs.putBool(wKeyStr, aValue);
        } else if constexpr (std::is_same<T, uint8_t>::value)   {
            wRetSize = mPrefs.putUChar(wKeyStr, aValue);
        } else if constexpr (std::is_same<T, uint16_t>::value)  {
            wRetSize = mPrefs.putUShort(wKeyStr, aValue);
        } else if constexpr (std::is_same<T, uint32_t>::value)  {
            wRetSize = mPrefs.putUInt(wKeyStr, aValue);
        } else if constexpr (std::is_same<T, int8_t>::value)    {
            wRetSize = mPrefs.putChar(wKeyStr, aValue);
        } else if constexpr (std::is_same<T, int16_t>::value)   {
            wRetSize = mPrefs.putShort(wKeyStr, aValue);
        } else if constexpr (std::is_same<T, int32_t>::value)   {
            wRetSize = mPrefs.putInt(wKeyStr, aValue);
        } else if constexpr (std::is_same<T, float>::value)     {
            wRetSize = mPrefs.putFloat(wKeyStr, aValue);
        } else if constexpr (std::is_same<T, double>::value)    {
            wRetSize = mPrefs.putDouble(wKeyStr, aValue);
        } else if constexpr (std::is_same<T, String>::value)    {
            wRetSize = mPrefs.putString(wKeyStr, aValue);
        }
        else
        {
            /* Type is not directly supported, generate  exception */
            static_assert(sizeof(T) == 0, "Unsupported type for SetValue() function");
        }

        mPrefs.end();
    }

    return (wRetSize == sizeof(T));
}

}   /* end of namespace SettingsNS */