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


};

extern Settings mSettings;

#endif /* SETTINGS_H_ */