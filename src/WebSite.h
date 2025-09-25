/*
 * WebSite.h
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#ifndef WEBSITE_H_
#define WEBSITE_H_

#include <Arduino.h>

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <EspWebUI.h>


class WebSite
{
public:
    WebSite();
    virtual ~WebSite();

    void Init(void);

    void Loop(void);

private:
    /* Pointer to this class */
    static WebSite* mpWebSite;
    /* Pointer to EspWebUI instance */
    EspWebUI* mpEspWebUI = nullptr;

    char mWebCallbackElementID[32];
    char mWebCallbackValue[256];
    bool mWebCallbackAvailable = false;

    void HandleReload(void);
    void HandleWebElement(const char *apElementID, const char *apElementValue);
    void HandleUpload(EspWebUI::uploadStatus aUploadState, const char *apMsg);
    void HandleOTA(EspWebUI::otaStatus aOtaState, const char *apMsg);


};

#endif /* WEBSITE_H_ */