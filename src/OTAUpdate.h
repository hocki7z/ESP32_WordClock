#pragma once

class AsyncWebServer;

namespace OTAUpdateNS
{
    /** Return the custom JavaScript injected into the ESPUI page. */
    const char* CustomJavaScript();

    /** Register OTA endpoints on the web server already started by ESPUI. */
    void RegisterRoutes(AsyncWebServer* apServer);
}
