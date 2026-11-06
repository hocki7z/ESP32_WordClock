/*
 * OTAUpdate.h
 *
 *  Created on: 02.11.2026
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

namespace OTAUpdateNS
{
    const char* CustomJavaScript(void);

    bool IsUploadActive(void);

    void RegisterRoutes(AsyncWebServer* apServer);
}
