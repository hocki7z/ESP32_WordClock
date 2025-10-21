/*
 * WebSite.cpp
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#include <ESPUI.h>

#include "Logger.h"
#include "Settings.hpp"

#include "WebSite.h"


/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)

/**
 * Initialize the private static pointer
 */
WebSite* WebSite::mpWebSiteInstance = nullptr;


/**
 * @brief Constructor
 */
WebSite::WebSite(char const* apName, ApplicationNS::tTaskPriority aPriority, const uint32_t aStackSize)
    : ApplicationNS::Task(apName, aPriority, aStackSize)
{
    /* Set "this" static pointer */
    mpWebSiteInstance = this;
}

/**
 * @brief Destructor
 */
WebSite::~WebSite()
{
    /* Clear "this" static pointer */
    mpWebSiteInstance = nullptr;
}

void WebSite::Init(ApplicationNS::tTaskObjects* apTaskObjects)
{
    /* Initialize base class */
    ApplicationNS::Task::Init(apTaskObjects);

    /* Initialize WEB UI but do not start it yet */

    /* Section Wordcolock settings */
    ESPUI.addControl(Control::Type::Separator, "Wordclock settings", "", Control::Color::Alizarin, Control::noParent);

    /* Clock mode */
    mWebUIControlID.mDisplayClockMode = AddSelectControl("Clock mode", mcClockModeItems, mcClockModeItemsCount,
            SettingsNS::mKeyDisplayClockMode, SettingsNS::mDefaultDisplayClockMode);

    /* Switcher for 'IT IS' words */
    mWebUIControlID.mDisplayClockItIs = AddSwitcherControl("Show 'IT IS'",
            SettingsNS::mKeyDisplayClockItIs, SettingsNS::mDefaultDisplayClockItIs);

    /* Switch for single minutes */
    mWebUIControlID.mDisplayClockSingleMinutes = AddSwitcherControl("Show single minutes",
            SettingsNS::mKeyDisplayClockSingleMins, SettingsNS::mDefaultDisplayClockSingleMins);

    /* Section LED settings */
    ESPUI.addControl(Control::Type::Separator, "LED settings", "", Control::Color::Alizarin, Control::noParent);

    /* Time color */
    mWebUIControlID.mDisplayColorTime = AddColorControl("Time color",
            SettingsNS::mKeyDisplayColorTime, SettingsNS::mDefaultDisplayColorTime);

    /* Background color */
    mWebUIControlID.mDisplayColorBackground = AddColorControl("Background color",
            SettingsNS::mKeyDisplayColorBkgd, SettingsNS::mDefaultDisplayColorBkgd);

}

void WebSite::ProcessIncomingMessage(const MessageNS::Message &arMessage)
{
    LOG(LOG_VERBOSE, "WebSite::ProcessIncomingMessage()");

    switch (arMessage.mId)
    {
        case MessageNS::tMessageId::MGS_STATUS_WIFI_STA_CONNECTED:
            /* WiFi connected, start WEB server */
            LOG(LOG_DEBUG, "WebSite::ProcessIncomingMessage() Start web server");

            ESPUI.begin("Wordclock");
            break;

        default:
            // do nothing
            break;
    }
}

void WebSite::HandleControl(Control* apControl, int aType)
{
    if (apControl->GetId() == mWebUIControlID.mDisplayClockMode)
    {
        /* Clock mode changed */
        HandleSelectControl(apControl, aType, SettingsNS::mKeyDisplayClockMode);
    }
    else if (apControl->GetId() == mWebUIControlID.mDisplayClockItIs)
    {
        /* 'IT IS' switcher changed */
        HandleSwitcherControl(apControl, aType, SettingsNS::mKeyDisplayClockItIs);
    }
    else if (apControl->GetId() == mWebUIControlID.mDisplayClockSingleMinutes)
    {
        /* Single minutes switcher changed */
        HandleSwitcherControl(apControl, aType, SettingsNS::mKeyDisplayClockSingleMins);
    }
    else if (apControl->GetId() == mWebUIControlID.mDisplayColorTime)
    {
        /* Time color changed */
        HandleColorControl(apControl, aType, SettingsNS::mKeyDisplayColorTime);
    }
    else if (apControl->GetId() == mWebUIControlID.mDisplayColorBackground)
    {
        /* Background color changed */
        HandleColorControl(apControl, aType, SettingsNS::mKeyDisplayColorBkgd);
    }
    else
    {
        LOG(LOG_ERROR, "WebSite::HandleControl() Unknown control ID %04X", apControl->GetId());
        return;
    }
}

Control::ControlId_t WebSite::AddColorControl(const char* apTitle, SettingsNS::tKey aSettingsKey, const uint32_t aDefaultColor)
{
    char wHexColor[10];
    uint32_t wColorParam = Settings.GetValue<uint32_t>(aSettingsKey, aDefaultColor);
    sprintf(wHexColor, "#%06X", (wColorParam & 0x00FFFFFF));

    Control::ControlId_t wControlId = ESPUI.text(apTitle, WebSite::ControlCallback, Control::Color::Dark, wHexColor);
    ESPUI.setInputType(wControlId, "color");

    LOG(LOG_DEBUG, "WebSite::AddColorControl() Control %04X, param 0x%08X, color %s",
        wControlId, wColorParam, String(wHexColor).c_str());

    return wControlId;
}

Control::ControlId_t WebSite::AddSwitcherControl(const char* apTitle, SettingsNS::tKey aSettingsKey, const bool aDefaultState)
{
    bool wState = Settings.GetValue<bool>(aSettingsKey, aDefaultState);

    Control::ControlId_t wControlId = ESPUI.switcher(apTitle, WebSite::ControlCallback, Control::Color::Dark, wState);

    LOG(LOG_DEBUG, "WebSite::AddSwitcherControl() Control %04X, param %s",
        wControlId, wState ? "ON" : "OFF");

    return wControlId;
}

Control::ControlId_t WebSite::AddSelectControl(const char* apTitle, const char* const* apItems, uint8_t aItemsCount,
        SettingsNS::tKey aSettingsKey, const uint8_t aDefaultOption)
{
    uint8_t wSelectedOption = Settings.GetValue<uint8_t>(aSettingsKey, aDefaultOption);

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Select, apTitle, "", Control::Color::Dark, Control::noParent, WebSite::ControlCallback);
    for (uint8_t wI = 0; wI < aItemsCount; wI++)
    {
        ESPUI.addControl(Control::Type::Option, apItems[wI], String(wI), Control::Color::None, wControlId);
    }
    ESPUI.updateSelect(wControlId, String(wSelectedOption));

    return wControlId;
}

void WebSite::HandleColorControl(Control* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    /* Retreive new color value */
    std::string wColorStr = aControl->value.c_str();
    uint32_t  wColorValue = std::stoi(wColorStr.substr(1), nullptr, 16); // skip '#'

    LOG(LOG_DEBUG, "WebSite::HandleColorControl() Control %04X, new color %s (0x%08X)",
            aControl->GetId(), wColorStr.c_str(), wColorValue);

    /* Store new color value in settings */
    Settings.SetValue<uint32_t>(aSettingsKey, wColorValue);
    /* Update displayed value */
    ESPUI.updateText(aControl->GetId(), aControl->value);
}

void WebSite::HandleSwitcherControl(Control* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    /* Retreive new switcher state */
    //bool wState = (aControl->value == "true") ? true : false;
    bool wState = (aType == S_ACTIVE) ? true : false;

    LOG(LOG_DEBUG, "WebSite::HandleSwitcherControl() Control %04X, new state %s",
            aControl->GetId(), wState ? "ON" : "OFF");

    /* Store new state in settings */
    Settings.SetValue<bool>(aSettingsKey, wState);
}

void WebSite::HandleSelectControl(Control* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    /* Retreive new selected option */
    uint8_t wSelectedOption = static_cast<uint8_t>(std::stoi(aControl->value.c_str()));

    LOG(LOG_DEBUG, "WebSite::HandleSelectControl() Control %04X, new selected option %d",
            aControl->GetId(), wSelectedOption);

    /* Store new selected option in settings */
    Settings.SetValue<uint8_t>(aSettingsKey, wSelectedOption);
}

void WebSite::ControlCallback(Control* apSender, int aType)
{
    if (mpWebSiteInstance)
    {
        mpWebSiteInstance->HandleControl(apSender, aType);
    }
}
