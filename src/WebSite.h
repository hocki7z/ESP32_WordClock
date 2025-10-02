/*
 * WebSite.h
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#ifndef WEBSITE_H_
#define WEBSITE_H_

#include <Arduino.h>
#include <WiFiGeneric.h>
#include <WiFiType.h>

#include <ESPUI.h>

typedef enum setting_type
{
    colorTime,
    colorBack,
    colorHour,
    colorMin1,
    colorMin2,
    colorMin3,
    colorMin4,
    usenightmode,
    day_time_start,
    day_time_stop,
    //
    setting_type_count
};

class WebSite
{
public:
    WebSite();
    virtual ~WebSite();

    void Init(void);

    void Loop(void);

private:
    void SetupColor(const setting_type aType, const char*  apCaption);

    void HandleWifiEvent(WiFiEvent_t aEvent);
};

#endif /* WEBSITE_H_ */