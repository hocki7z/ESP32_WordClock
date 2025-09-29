/*
 * WebSite.cpp
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#include "Logger.h"
#include "WebSite.h"


/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)

/**
 * @brief Constructor
 */
WebSite::WebSite()
{
    // nothig to do
}

/**
 * @brief Destructor
 */
WebSite::~WebSite()
{
    // nothig to do
}

void WebSite::Init(void)
{
    /* Register WiFi events listener */
	WiFi.onEvent(
        std::bind(&WebSite::HandleWifiEvent, this, std::placeholders::_1));
}

void WebSite::Loop(void)
{
    // nothig to do
}

void WebSite::HandleWifiEvent(WiFiEvent_t aEvent)
{
	switch (aEvent)
	{
		case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            /* LOG */
            LOG(LOG_DEBUG, "WebSite::HandleWifiEvent() Start web server");
            /* Start web server */
            ESPUI.begin("Wordclock");
			break;

		default:
			break;
    }
}