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
 *      int myInt = Settings.GetValue<int>("myIntKey", 42);
 *      bool myBool = Settings.GetValue<bool>("myBoolKey", false);
 *      String myString = Settings.GetValue<String>("myStringKey", "default");
 *
 *   Unsupported type (will cause compile-time error):
 *      std::vector<int> myVector = Settings.GetValue<std::vector<int>>("myVectorKey", {});
 */
template<typename T>
T Settings::GetValue(tKey arKey, const T aDefaultValue)
{
    T wRetValue = aDefaultValue;

    /* Check if key exists and open preferences in read-only mode */
    if ((HasKey(arKey) == true) &&
        (mPrefs.begin(mcPrefsParamNamespace, true)))
    {
        if constexpr (std::is_same<T, bool>::value) {
            wRetValue = mPrefs.getBool(arKey, aDefaultValue);
        } else if constexpr (std::is_same<T, uint8_t>::value)   {
            wRetValue = mPrefs.getUChar(arKey, aDefaultValue);
        } else if constexpr (std::is_same<T, uint16_t>::value)  {
            wRetValue = mPrefs.getUShort(arKey, aDefaultValue);
        } else if constexpr (std::is_same<T, uint32_t>::value)  {
            wRetValue = mPrefs.getUInt(arKey, aDefaultValue);
        } else if constexpr (std::is_same<T, int8_t>::value)    {
            wRetValue = mPrefs.getChar(arKey, aDefaultValue);
        } else if constexpr (std::is_same<T, int16_t>::value)   {
            wRetValue = mPrefs.getShort(arKey, aDefaultValue);
        } else if constexpr (std::is_same<T, int32_t>::value)   {
            wRetValue = mPrefs.getInt(arKey, aDefaultValue);
        } else if constexpr (std::is_same<T, float>::value)     {
            wRetValue = mPrefs.getFloat(arKey, aDefaultValue);
        } else if constexpr (std::is_same<T, double>::value)    {
            wRetValue = mPrefs.getDouble(arKey, aDefaultValue);
        } else if constexpr (std::is_same<T, String>::value)    {
            wRetValue = mPrefs.getString(arKey, aDefaultValue);
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
 *      bool success = Settings.SetValue<int>("myIntKey", 42);
 *      bool success = Settings.SetValue<bool>("myBoolKey", true);
 *      bool success = Settings.SetValue<String>("myStringKey", "Hello");
 *
 *    Unsupported type (will cause compile-time error):
 *     bool success = settings.SetValue<std::vector<int>>("myVectorKey", {1,2,3});
 */
template<typename T>
bool Settings::SetValue(tKey arKey, const T aValue)
{
    size_t wRetSize = 0;

    /* Open preferences in read-write mode */
    if (mPrefs.begin(mcPrefsParamNamespace, false))
    {
        if constexpr (std::is_same<T, bool>::value) {
            wRetSize = mPrefs.putBool(arKey, aValue);
        } else if constexpr (std::is_same<T, uint8_t>::value)   {
            wRetSize = mPrefs.putUChar(arKey, aValue);
        } else if constexpr (std::is_same<T, uint16_t>::value)  {
            wRetSize = mPrefs.putUShort(arKey, aValue);
        } else if constexpr (std::is_same<T, uint32_t>::value)  {
            wRetSize = mPrefs.putUInt(arKey, aValue);
        } else if constexpr (std::is_same<T, int8_t>::value)    {
            wRetSize = mPrefs.putChar(arKey, aValue);
        } else if constexpr (std::is_same<T, int16_t>::value)   {
            wRetSize = mPrefs.putShort(arKey, aValue);
        } else if constexpr (std::is_same<T, int32_t>::value)   {
            wRetSize = mPrefs.putInt(arKey, aValue);
        } else if constexpr (std::is_same<T, float>::value)     {
            wRetSize = mPrefs.putFloat(arKey, aValue);
        } else if constexpr (std::is_same<T, double>::value)    {
            wRetSize = mPrefs.putDouble(arKey, aValue);
        } else if constexpr (std::is_same<T, String>::value)    {
            wRetSize = mPrefs.putString(arKey, aValue);
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