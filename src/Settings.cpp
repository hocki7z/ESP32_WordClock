/*
 * Settings.cpp
 *
 *  Created on: 05.10.2025
 *      Author: hocki
 */

#include "Settings.h"


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

void Settings::Init(void)
{
}

void Settings::Loop(void)
{
}

/**
 * @brief Returns the current value of the property with the given name
 *
 * @param arName the name of the property
 *
 * @return the property value
 */
String Settings::GetValue(const String& arName)
{
    return GetValue(arName, "");
}

/**
 * @brief Returns the current value of the property with the given name
 *
 * @param parName the name of the property
 * @param parDefaultValue default value
 *
 * @return the property value or default value when property does not exist.
 */
String Settings::GetValue(const String& arName, String aDefaultValue)
{
    String wRetValue = "";

    /* Start a namespace "prefs" */
    if (mPrefs.begin(mcPrefsName, true)) //true - read only mode
    {
        /* Get value of key , if key not exist return default value in second argument */
        wRetValue = mPrefs.getString(arName.c_str(), aDefaultValue);
        /* Close the Preferences */
        mPrefs.end();
    }
    return wRetValue;
}

/**
 * @brief Sets the current value of property with the given name
 */
bool Settings::SetValue(const String& arName, String aValue)
{
    bool wRetValue = false;

    /* Start a namespace "prefs" */
    if (mPrefs.begin(mcPrefsName, false)) //false - read-write mode
    {
        /* Store the value to the preferences */
        wRetValue = (mPrefs.putString(arName.c_str(), aValue.c_str())) > 0;
        /* Close the Preferences */
        mPrefs.end();
    }
    return wRetValue;
}

bool Settings::SetValue(const String& arName, uint32_t aIndex, String aValue)
{
    return SetValue(arName + String(aIndex), aValue);
}

/**
 * @ brief Removes the property with the given name.
 */
bool Settings::DelProperty(const String& arName)
{
    bool wRetValue = false;

    /* Start a namespace "prefs" */
    if (mPrefs.begin(mcPrefsName, false)) //false - read-write mode
    {
        /* Remove the key  */
        wRetValue = mPrefs.remove(arName.c_str());

        /* Close the Preferences */
        mPrefs.end();
    }

    return wRetValue;
}

bool Settings::DelProperty(const String& arName, uint32_t aIndex)
{
    return DelProperty(arName + String(aIndex));
}

/**
 * @brief Checks if a property with the specified name exists.
 */
bool Settings::HasProperty(const String& arName)
{
    return GetValue(arName).length() != 0;
}

//bool Settings::HasProperty(const String& arName, uint32_t aIndex)
//{
//    return GetValue(arName, aIndex, "").length() != 0;
//}

/* Instance of Settings class */
Settings mSettings;