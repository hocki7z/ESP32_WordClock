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


class WebSite
{
public:
    WebSite();
    virtual ~WebSite();

    void Init(void);

    void Loop(void);

private:

    void HandleWifiEvent(WiFiEvent_t aEvent);
};

#endif /* WEBSITE_H_ */