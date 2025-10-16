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
WebSite::WebSite(char const* apName, ApplicationNS::tTaskPriority aPriority, const uint32_t aStackSize)
    : ApplicationNS::Task(apName, aPriority, aStackSize)
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

void WebSite::Init(ApplicationNS::tTaskObjects* apTaskObjects)
{
    /* Initialize base class */
    ApplicationNS::Task::Init(apTaskObjects);

    /* Create async web server */
    mpWebServer = new AsyncWebServer(80);
    /* and server routes */
    mpWebServer->on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(200, "text/plain", "Wordclock");
    });
}

void WebSite::ProcessIncomingMessage(const MessageNS::Message &arMessage)
{
    LOG(LOG_VERBOSE, "WebSite::ProcessIncomingMessage()");

    switch (arMessage.mId)
    {
        case MessageNS::tMessageId::MGS_STATUS_WIFI_STA_CONNECTED:
            /* WiFi connected, start WEB server */
            LOG(LOG_DEBUG, "WebSite::ProcessIncomingMessage() Start web server");
            mpWebServer->begin();
            break;

        default:
            // do nothing
            break;
    }
}