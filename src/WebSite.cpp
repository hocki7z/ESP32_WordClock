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

#define HTTP_PORT   (80)

/* Initialize the private static pointer */
WebSite* WebSite::mpWebSite = nullptr;


/**
 * @brief Constructor
 */
WebSite::WebSite()
{
    /* Set "this" static pointer */
    mpWebSite = this;
}

/**
 * @brief Destructor
 */
WebSite::~WebSite()
{
    /* Clear pointers */
    mpWebSite = nullptr;
}

void WebSite::Init(void)
{
    mpEspWebUI = new EspWebUI(HTTP_PORT);

    /* Setup callback functions */
    mpEspWebUI->setCallbackReload([]() {
        if (mpWebSite != nullptr)
            mpWebSite->HandleReload();
    });
    mpEspWebUI->setCallbackWebElement([](const char *apElementID, const char *apElementValue) {
        if (mpWebSite != nullptr)
        {
            /* Copy element ID and value to member variables */
            snprintf(mpWebSite->mWebCallbackElementID, sizeof(mpWebSite->mWebCallbackElementID), "%s", apElementID);
            snprintf(mpWebSite->mWebCallbackValue, sizeof(mpWebSite->mWebCallbackValue), "%s", apElementValue);
            /* Set flag that a new web element is available */
            mpWebSite->mWebCallbackAvailable = true;
        }
    });
    mpEspWebUI->setCallbackUpload([](EspWebUI::uploadStatus aUploadState, const char *apMsg) {
        if (mpWebSite != nullptr)
            mpWebSite->HandleUpload(aUploadState, apMsg);
    });
    mpEspWebUI->setCallbackOta([](EspWebUI::otaStatus aOtaState, const char *apMsg) {
        if (mpWebSite != nullptr)
            mpWebSite->HandleOTA(aOtaState, apMsg);
    });

    /* Setup authentication */
    mpEspWebUI->setAuthentication(false);
//    mpEspWebUI->setCredentials(username, password);

    /* Start web server */
    mpEspWebUI->begin();
}

void WebSite::Loop(void)
{
    mpEspWebUI->loop();
}

void WebSite::HandleReload(void)
{
}

void WebSite::HandleWebElement(const char *apElementID, const char *apElementValue)
{
}

void WebSite::HandleUpload(EspWebUI::uploadStatus aUploadState, const char *apMsg)
{
}

void WebSite::HandleOTA(EspWebUI::otaStatus aOtaState, const char *apMsg)
{
}
