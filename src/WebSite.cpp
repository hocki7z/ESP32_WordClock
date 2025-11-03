/*
 * WebSite.cpp
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#include <WiFi.h>
#include <ESPUI.h>

#include "Logger.h"
#include "DateTime.h"
#include "Settings.hpp"

#include "WebSite.h"


/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)


inline String IpAddress2String(const IPAddress& arIpAddress)
{
    return String(arIpAddress[0]) + String(".") + String(arIpAddress[1]) + String(".") +
           String(arIpAddress[2]) + String(".") + String(arIpAddress[3]);
}

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

    mpConfigWebServer = new AsyncWebServer(2025);
}

/**
 * @brief Destructor
 */
WebSite::~WebSite()
{
    /* Delete web server instance */
    delete mpConfigWebServer;
    mpConfigWebServer = nullptr;

    /* Clear "this" static pointer */
    mpWebSiteInstance = nullptr;
}

void WebSite::Init(ApplicationNS::tTaskObjects* apTaskObjects)
{
    /* Initialize base class */
    ApplicationNS::Task::Init(apTaskObjects);

    /* Initialize WEB UI but do not start it yet */

    /* ESPUI Log mode */
    ESPUI.setVerbosity(Verbosity::Verbose);

    /* Section Wordcolock settings */
    ESPUI.addControl(Control::Type::Separator, "Wordclock settings", "", Control::Color::Alizarin, Control::noParent);

    /* Clock mode */
    mWebUIControlID.mDisplayClockMode = AddSelectControl("Clock mode", ConfigNS::mcClockModeItems, ConfigNS::mcClockModeItemsCount,
            ConfigNS::mKeyDisplayClockMode, ConfigNS::mDefaultDisplayClockMode);

    /* Switcher for 'IT IS' words */
    mWebUIControlID.mDisplayClockItIs = AddSwitcherControl("Show 'IT IS'",
            ConfigNS::mKeyDisplayClockItIs, ConfigNS::mDefaultDisplayClockItIs);

    /* Switch for single minutes */
    mWebUIControlID.mDisplayClockSingleMinutes = AddSwitcherControl("Show single minutes",
            ConfigNS::mKeyDisplayClockSingleMins, ConfigNS::mDefaultDisplayClockSingleMins);

    /* Section LED settings */
    ESPUI.addControl(Control::Type::Separator, "LED colors", "", Control::Color::Alizarin, Control::noParent);

    /* Time color */
    mWebUIControlID.mDisplayColorTime = AddColorControl("Time color",
            ConfigNS::mKeyDisplayColorTime, ConfigNS::mDefaultDisplayColorTime);

    /* Background color */
    mWebUIControlID.mDisplayColorBackground = AddColorControl("Background color",
            ConfigNS::mKeyDisplayColorBkgd, ConfigNS::mDefaultDisplayColorBkgd);

    /* Day/Night settings */
    ESPUI.addControl(Control::Type::Separator, "LED brightness", "", Control::Color::Alizarin, Control::noParent);
    /* Slider for LED brightness selection */
    mWebUIControlID.mDisplayLedBrightness = AddPercentageSliderControl("LED brightness",
            ConfigNS::mKeyDisplayLedBrightness, ConfigNS::mDefaultDisplayLedBrightness);
    /* Switcher for day/night mode activation */
    mWebUIControlID.mDisplayUseNightMode = AddSwitcherControl("Use day/night mode",
            ConfigNS::mKeyDisplayUseNightMode, ConfigNS::mDefaultDisplayUseNightMode);
    /* Slider for night brightness selection */
    mWebUIControlID.mDisplayBrightnessNightMode = AddPercentageSliderControl("Night mode brightness",
            ConfigNS::mKeyDisplayBrightnessNightMode, ConfigNS::mDefaultDisplayBrightnessNightMode);
    /* Time input for night mode start */
    mWebUIControlID.mDisplayNightModeStartTime = AddTimeControl("Night mode start time",
            ConfigNS::mKeyDisplayNightModeStartTime, ConfigNS::mDefaultDisplayNightModeStartTime);
    /* Time input for night mode end */
    mWebUIControlID.mDisplayNightModeEndTime = AddTimeControl("Night mode end time",
            ConfigNS::mKeyDisplayNightModeEndTime, ConfigNS::mDefaultDisplayNightModeEndTime);


    /* Section DateTime settings */
    ESPUI.addControl(Control::Type::Separator, "DateTime settings", "", Control::Color::Alizarin, Control::noParent);
    /* NTP server selection */
    mWebUIControlID.mDatetimeNtpServer = AddSelectControl("NTP server",
            ConfigNS::mcNtpServerItems, ConfigNS::mcNtpServerItemsCount,
            ConfigNS::mKeyNtpServer, ConfigNS::mDefaultNtpServer);
    /* Timezone selection */
    mWebUIControlID.mDatetimeTimeZone = AddSelectControl("Time zone",
            ConfigNS::mcTimezoneNames, ConfigNS::mcTimezoneItemsCount,
            ConfigNS::mKeyTimeZone, ConfigNS::mDefaultTimeZone);

    /* Update LED brightness controls */
    UpdateLedBrightnessControls();

    mpConfigWebServer->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String message = "WordClock web configuration interface.\n\n";
        message = message + "General:\n";
        message = message + "http://" + IpAddress2String(WiFi.localIP()) + ":2025 --> Shows this text\n\n";

        request->send(200, "text/plain", message);
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

            ESPUI.begin("Wordclock");
            mpConfigWebServer->begin();
            break;

            case MessageNS::tMessageId::MSG_EVENT_SETTINGS_CHANGED:
            {
                /* Check if message sent by this task */
                if (arMessage.mSource == MessageNS::tAddress::WEB_MANAGER)
                {
                    /* Update LED brightness controls */
                    UpdateLedBrightnessControls();

                    /* Redirect this message to display manager */
                    MessageNS::Message wMessage = arMessage;

                    /* Send message to display */
                    wMessage.mDestination = MessageNS::tAddress::DISPLAY_MANAGER;
                    mpTaskObjects->mpCommunicationManager->SendMessage(wMessage);

                    /* Send message to time manager */
                    wMessage.mDestination = MessageNS::tAddress::TIME_MANAGER;
                    mpTaskObjects->mpCommunicationManager->SendMessage(wMessage);

                    break;
                }
            }
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
        HandleSelectControl(apControl, aType, ConfigNS::mKeyDisplayClockMode);
    }
    else if (apControl->GetId() == mWebUIControlID.mDisplayClockItIs)
    {
        /* 'IT IS' switcher changed */
        HandleSwitcherControl(apControl, aType, ConfigNS::mKeyDisplayClockItIs);
    }
    else if (apControl->GetId() == mWebUIControlID.mDisplayClockSingleMinutes)
    {
        /* Single minutes switcher changed */
        HandleSwitcherControl(apControl, aType, ConfigNS::mKeyDisplayClockSingleMins);
    }
    else if (apControl->GetId() == mWebUIControlID.mDisplayColorTime)
    {
        /* Time color changed */
        HandleColorControl(apControl, aType, ConfigNS::mKeyDisplayColorTime);
    }
    else if (apControl->GetId() == mWebUIControlID.mDisplayColorBackground)
    {
        /* Background color changed */
        HandleColorControl(apControl, aType, ConfigNS::mKeyDisplayColorBkgd);
    }
    else if (apControl->GetId() == mWebUIControlID.mDisplayUseNightMode)
    {
        /* Day/night mode switcher changed */
        HandleSwitcherControl(apControl, aType, ConfigNS::mKeyDisplayUseNightMode);
    }
    else if (apControl->GetId() == mWebUIControlID.mDisplayLedBrightness)
    {
        /* Led brightness slider changed */
        HandlePercentageSliderControl(apControl, aType, ConfigNS::mKeyDisplayLedBrightness);
    }
    else if (apControl->GetId() == mWebUIControlID.mDisplayBrightnessNightMode)
    {
        /* Night mode brightness slider changed */
        HandlePercentageSliderControl(apControl, aType, ConfigNS::mKeyDisplayBrightnessNightMode);
    }
    else if (apControl->GetId() == mWebUIControlID.mDisplayNightModeStartTime)
    {
        /* Day mode start time changed */
        HandleTimerControl(apControl, aType, ConfigNS::mKeyDisplayNightModeStartTime);
    }
    else if (apControl->GetId() == mWebUIControlID.mDisplayNightModeEndTime)
    {
        /* Day mode end time changed */
        HandleTimerControl(apControl, aType, ConfigNS::mKeyDisplayNightModeEndTime);
    }
    else if (apControl->GetId() == mWebUIControlID.mDatetimeNtpServer)
    {
        /* NTP server selection changed */
        HandleSelectControl(apControl, aType, ConfigNS::mKeyNtpServer);
    }
    else if (apControl->GetId() == mWebUIControlID.mDatetimeTimeZone)
    {
        /* Timezone selection changed */
        HandleSelectControl(apControl, aType, ConfigNS::mKeyTimeZone);
    }

    else
    {
        LOG(LOG_ERROR, "WebSite::HandleControl() Unknown control ID %04X", apControl->GetId());
        return;
    }

    /* Create message */
    MessageNS::Message wMessage;
    wMessage.mSource = MessageNS::tAddress::WEB_MANAGER;
    wMessage.mDestination = MessageNS::tAddress::WEB_MANAGER;

    wMessage.mId = MessageNS::tMessageId::MSG_EVENT_SETTINGS_CHANGED;

    /* Send message */
    mpTaskObjects->mpCommunicationManager->SendMessage(wMessage);
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

Control::ControlId_t WebSite::AddPercentageSliderControl(const char* apTitle, SettingsNS::tKey aSettingsKey, const uint8_t aDefaultValue)
{
    uint8_t wValue = Settings.GetValue<uint8_t>(aSettingsKey, aDefaultValue);

    Control::ControlId_t wControlId = ESPUI.slider(apTitle, WebSite::ControlCallback, Control::Color::Dark, wValue, 0, 100);

    LOG(LOG_DEBUG, "WebSite::AddPercentageSliderControl() Control %04X, value %d", wControlId, wValue);

    return wControlId;
}

Control::ControlId_t WebSite::AddTimeControl(const char* apTitle, SettingsNS::tKey aSettingsKey, const uint32_t aDefaultTime)
{
    char wTimeStr[6];

    uint32_t wTimeInt = Settings.GetValue<uint32_t>(aSettingsKey, aDefaultTime);
    DateTimeNS::tDateTime wDateTime = DateTimeNS::DwordToDateTime(wTimeInt);

    /* Convert time to string format HH:MM */
    sprintf(wTimeStr, "%02u:%02u", wDateTime.mTime.mHour, wDateTime.mTime.mMinute);

    Control::ControlId_t wControlId = ESPUI.text(apTitle, WebSite::ControlCallback, Control::Color::Dark, wTimeStr);
    ESPUI.setInputType(wControlId, "time");

    LOG(LOG_DEBUG, "WebSite::AddTimeControl() Control %04X, time %s", wControlId, String(wTimeStr).c_str());

    return wControlId;
}

void WebSite::UpdateLedBrightnessControls(bool aForceUpdate)
{
    bool wUseNightMode = Settings.GetValue<bool>(ConfigNS::mKeyDisplayUseNightMode, ConfigNS::mDefaultDisplayUseNightMode);

    LOG(LOG_DEBUG, "WebSite::UpdateLedBrightnessControls() Use night mode %d, force update %d", wUseNightMode, aForceUpdate);

    /* Issue:
          The UI controls have not been displayed properly after
          the visibility was changed from false to true.

          The UI controls in the web interface do not display correctly after
          their visibility is changed (e.g. from false to true).

       Workaround:
          Instead of changing the visibility of the controls, enable/disable them. */

    // ESPUI.updateVisibility(mWebUIControlID.mDisplayBrightnessNightMode, wUseNightMode);
    // ESPUI.updateVisibility(mWebUIControlID.mDisplayNightModeStartTime,  wUseNightMode);
    // ESPUI.updateVisibility(mWebUIControlID.mDisplayNightModeEndTime,    wUseNightMode);

    ESPUI.setEnabled(mWebUIControlID.mDisplayBrightnessNightMode, wUseNightMode);
    ESPUI.setEnabled(mWebUIControlID.mDisplayNightModeStartTime,  wUseNightMode);
    ESPUI.setEnabled(mWebUIControlID.mDisplayNightModeEndTime,    wUseNightMode);

    if (aForceUpdate)
    {
        ESPUI.jsonReload();
    }
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

    LOG(LOG_DEBUG, "WebSite::HandleSwitcherControl() Control %04X, new state %d",
            aControl->GetId(), wState);

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

void WebSite::HandlePercentageSliderControl(Control* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    /* Retreive new slider value */
    uint8_t wValue = static_cast<uint8_t>(std::stoi(aControl->value.c_str()));

    LOG(LOG_DEBUG, "WebSite::HandlePercentageSliderControl() Control %04X, new value %d",
            aControl->GetId(), wValue);

    /* Store new slider value in settings */
    Settings.SetValue<uint8_t>(aSettingsKey, wValue);
}

void WebSite::HandleTimerControl(Control* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    /* Retreive new time value in format HH:MM */
    std::string wTimeStr = aControl->value.c_str();

    uint8_t wHour   = static_cast<uint8_t>(std::stoi(wTimeStr.substr(0, 2)));
    uint8_t wMinute = static_cast<uint8_t>(std::stoi(wTimeStr.substr(3, 2)));

    LOG(LOG_DEBUG, "WebSite::HandleTimerControl() Control %04X, new time %02u:%02u",
            aControl->GetId(), wHour, wMinute);

    /* Store new time value in settings */
    DateTimeNS::tDateTime wDateTime;
    wDateTime.mTime.mHour   = wHour;
    wDateTime.mTime.mMinute = wMinute;
    wDateTime.mTime.mSecond = 0;
    wDateTime.mDate.mDay    = 1;
    wDateTime.mDate.mMonth  = 1;
    wDateTime.mDate.mYear   = DateTimeNS::mYearRangeStart;

    uint32_t wTimeDword = DateTimeNS::DateTimeToDword(wDateTime);
    Settings.SetValue<uint32_t>(aSettingsKey, wTimeDword);
}

void WebSite::ControlCallback(Control* apSender, int aType)
{
    if (mpWebSiteInstance)
    {
        mpWebSiteInstance->HandleControl(apSender, aType);
    }
}
