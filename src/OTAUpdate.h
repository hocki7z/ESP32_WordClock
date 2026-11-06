/*
 * OTAUpdate.h
 *
 *  Created on: 02.11.2026
 *      Author: hocki
 */
#pragma once

class AsyncWebServer;

namespace OTAUpdateNS
{
    /** Return the custom JavaScript injected into the ESPUI page. */
    const char* CustomJavaScript();

    /** Return whether an OTA upload is currently being received or validated. */
    bool IsUploadActive();

    /** Register OTA endpoints on the web server already started by ESPUI. */
    void RegisterRoutes(AsyncWebServer* apServer);
}
