/*
 * Settings.h
 *
 *  Created on: 05.10.2025
 *      Author: hocki
 */
#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <Arduino.h>
#include <Preferences.h>


class Settings
{
public:
    Settings();
    virtual ~Settings();

    void Init(void);

    void Loop(void);

    void ClearAll(void);

    String GetValue(const String& arName);
    String GetValue(const String& arName, String aDefaultValue);

    bool SetValue(const String& arName, String aDefaultValue);
    bool SetValue(const String& arName, uint32_t aIndex, String aDefaultValue);

    bool DelProperty(const String& arName);
    bool DelProperty(const String& arName, uint32_t aIndex);

    bool HasProperty(const String& arName);
    //bool HasProperty(const String& aNarNameame, uint32_t aIndex);

    template<typename T> String GetValue(const String& arName, T aDefaultValue)
    {
        return GetValue(arName, String(aDefaultValue));
    }

    template<typename T> String getValue(const String& arName, uint32_t aIndex, T aDefaultValue)
    {
        return GetValue(arName + String(aIndex), aDefaultValue);
    }

    template<typename T> bool SetValue(const String& arName, T aValue)
    {
        return SetValue(arName, String(aValue));
    }

    template<typename T> bool setValue(const String& arName, uint32_t aIndex, T aValue)
    {
        return SetValue(arName + String(aIndex), aValue);
    }

private:

    static constexpr const char* mcPrefsName = "prefs";

    /* Instance of preferences */
    Preferences mPrefs;

};

extern Settings mSettings;

#endif /* SETTINGS_H_ */