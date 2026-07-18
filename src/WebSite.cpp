/*
 * WebSite.cpp
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#include <ESPUI.h>

#include "Logger.h"
#include "DateTime.h"
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

    /* ESPUI Log mode */
    ESPUI.setVerbosity(Verbosity::Verbose);

    /* Section Wordcolock settings */
    ESPUI.addControl(Control::Type::Separator, "Wordclock settings", "", Control::Color::Alizarin, Control::noParent);

    /* Clock mode */
    mWebUIControlID.mDisplayClockMode = AddSelectControl("Clock mode", Control::noParent, "",
            ConfigNS::mcClockModeItems, ConfigNS::mcClockModeItemsCount,
            ConfigNS::mKeyDisplayClockMode, ConfigNS::mDefaultDisplayClockMode);

    /* Switcher for 'IT IS' words */
    mWebUIControlID.mDisplayClockItIs = AddSwitcherControl("Show 'IT IS'", Control::noParent, "",
            ConfigNS::mKeyDisplayClockItIs, ConfigNS::mDefaultDisplayClockItIs);

    /* Switch for single minutes */
    mWebUIControlID.mDisplayClockSingleMinutes = AddSwitcherControl("Show single minutes", Control::noParent, "",
            ConfigNS::mKeyDisplayClockSingleMins, ConfigNS::mDefaultDisplayClockSingleMins);

    /* Section LED settings */
    ESPUI.addControl(Control::Type::Separator, "LED colors", "", Control::Color::Alizarin, Control::noParent);

    /* Time color */
    mWebUIControlID.mDisplayColorTime = AddColorControl("Time color", Control::noParent, "",
            ConfigNS::mKeyDisplayColorTime, ConfigNS::mDefaultDisplayColorTime);

    /* Background color */
    mWebUIControlID.mDisplayColorBackground = AddColorControl("Background color", Control::noParent, "",
            ConfigNS::mKeyDisplayColorBkgd, ConfigNS::mDefaultDisplayColorBkgd);

    /* Day/Night settings */
    ESPUI.addControl(Control::Type::Separator, "LED brightness", "", Control::Color::Alizarin, Control::noParent);
    /* Slider for LED brightness selection */
    mWebUIControlID.mDisplayLedBrightness = AddPercentageSliderControl("LED brightness", Control::noParent, "",
            ConfigNS::mKeyDisplayLedBrightness, ConfigNS::mDefaultDisplayLedBrightness);
    /* Switcher for day/night mode activation */
    mWebUIControlID.mDisplayUseNightMode = AddSwitcherControl("Use day/night mode", Control::noParent, "",
            ConfigNS::mKeyDisplayUseNightMode, ConfigNS::mDefaultDisplayUseNightMode);
    /* Slider for night brightness selection */
    mWebUIControlID.mDisplayBrightnessNightMode = AddPercentageSliderControl("Night mode brightness", Control::noParent, "",
            ConfigNS::mKeyDisplayBrightnessNightMode, ConfigNS::mDefaultDisplayBrightnessNightMode);
    /* Time input for night mode start */
    mWebUIControlID.mDisplayNightModeStartTime = AddTimeControl("Night mode start time", Control::noParent, "",
            ConfigNS::mKeyDisplayNightModeStartTime, ConfigNS::mDefaultDisplayNightModeStartTime);
    /* Time input for night mode end */
    mWebUIControlID.mDisplayNightModeEndTime = AddTimeControl("Night mode end time", Control::noParent, "",
            ConfigNS::mKeyDisplayNightModeEndTime, ConfigNS::mDefaultDisplayNightModeEndTime);


    /* Section DateTime settings */
    ESPUI.addControl(Control::Type::Separator, "DateTime settings", "", Control::Color::Alizarin, Control::noParent);
    /* NTP server selection */
    mWebUIControlID.mDatetimeNtpServer = AddSelectControl("NTP server", Control::noParent, "",
            ConfigNS::mcNtpServerItems, ConfigNS::mcNtpServerItemsCount,
            ConfigNS::mKeyNtpServer, ConfigNS::mDefaultNtpServer);
    /* Timezone selection */
    mWebUIControlID.mDatetimeTimeZone = AddSelectControl("Time zone", Control::noParent, "",
            ConfigNS::mcTimezoneNames, ConfigNS::mcTimezoneItemsCount,
            ConfigNS::mKeyTimeZone, ConfigNS::mDefaultTimeZone);

    /* Section WiFi settings */
    ESPUI.addControl(Control::Type::Separator, "WiFi settings", "", Control::Color::Alizarin, Control::noParent);

    // Add WiFi settings controls here (e.g., SSID, password, etc.)
    mWebUIControlID.mWifiSSIDs = AddSelectControl("SSID");

    mWebUIControlID.mWifiPassword = AddPasswordControl("Password");
    mWebUIControlID.mWifiPasswordShowHide = AddSwitcherControl("Show/Hide Password",  Control::noParent, "",
        ConfigNS::mKeyWifiPassword, false);

    // Add buttons for scanning WiFi networks and connecting to the selected network
    mWebUIControlID.mWifiConnectButton = AddButtonControl("Connect to selected network", "Save & Connect");
    mWebUIControlID.mWifiScanButton = AddButtonControl("Scan WiFi networks", "Search for WiFi");


    /* Update LED brightness controls */
    UpdateLedBrightnessControls();
}

void WebSite::ProcessIncomingMessage(const MessageNS::Message &arMessage)
{
    LOG(LOG_VERBOSE, "WebSite::ProcessIncomingMessage()");

    switch (arMessage.mId)
    {
        case MessageNS::tMessageId::MSG_EVENT_WIFI_STA_CONNECTED:
            /* WiFi connected, start WEB server */
            LOG(LOG_DEBUG, "WebSite::ProcessIncomingMessage() Start web server");

            /* Normal Mode - no captive portal */
            ESPUI.captivePortal = false;
            /* Start WEB UI */
            ESPUI.begin("Wordclock");
            break;

        case MessageNS::tMessageId::MSG_EVENT_WIFI_AP_STARTED:
            /* Access point started, start WEB server */
            LOG(LOG_DEBUG, "WebSite::ProcessIncomingMessage() Start web server in AP mode");

            /* Offline Mode - enable captive portal */
            ESPUI.captivePortal = true;
            /* Start WEB UI */
            ESPUI.begin("Wordclock");
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
                SendMessage(wMessage);

                /* Send message to time manager */
                wMessage.mDestination = MessageNS::tAddress::TIME_MANAGER;
                SendMessage(wMessage);

                break;
            }
        }
            break;

        case MessageNS::tMessageId::MSG_EVENT_WIFI_SCAN_DONE:
            /* WiFi scan finished, update WiFi settings */
            UpdateWiFiSettingsControls();
            break;

        default:
            // do nothing
            break;
    }
}

void WebSite::HandleControl(BasicControl* apControl, int aType, void* apParam)
{
    MessageNS::Message wMessage;

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
    else if (apControl->GetId() == mWebUIControlID.mWifiSSIDs)
    {
        /* WiFi SSID selection changed */

        // Get selected SSID and password from the controls
        uint8_t wSsidIndex = ESPUI.getControl(mWebUIControlID.mWifiSSIDs)->getValueInt();

        if (wSsidIndex < mLocalSsidList.size() && (mLocalSsidList[wSsidIndex].mEncrypted))
        {
            String wPassw = ESPUI.getControl(mWebUIControlID.mWifiPassword)->getValue().c_str();

            // De-/activate button mWifiConnectButton
            ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton, (wPassw.length() > 0));
        }

        return;
    }
    else if (apControl->GetId() == mWebUIControlID.mWifiPassword)
    {
        /* WiFi password changed */
        String wPassw = ESPUI.getControl(mWebUIControlID.mWifiPassword)->getValue().c_str();
        
        // De-/activate button mWifiConnectButton
        ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton, (wPassw.length() > 0));

        return;
    }
    else if (apControl->GetId() == mWebUIControlID.mWifiPasswordShowHide)
    {
        /* WiFi password show/hide switcher changed */
        bool wState = (aType == S_ACTIVE) ? true : false;

        if (wState)
        {
            // Show password
            ESPUI.setInputType(mWebUIControlID.mWifiPassword, "text");
        }
        else
        {
            // Hide password
            ESPUI.setInputType(mWebUIControlID.mWifiPassword, "password");
        }

        return;
    }
    else if (apControl->GetId() == mWebUIControlID.mWifiConnectButton)
    {
        /* Connect to selected network button pressed */
        if (aType == B_UP)
        {
            LOG(LOG_DEBUG, "WebSite::HandleWiFiSettingsControls() WiFi connect button pressed");

            // Implement connect logic using selected SSID and password
            wifi_config_t wifi_config = {0};

            // Get selected SSID and password from the controls
            uint8_t wSsidIndex = ESPUI.getControl(mWebUIControlID.mWifiSSIDs)->getValueInt();
            String wSsid  = mLocalSsidList[wSsidIndex].mSsid;
            String wPassw = ESPUI.getControl(mWebUIControlID.mWifiPassword)->getValue().c_str();

            LOG(LOG_DEBUG, "WebSite::HandleWiFiSettingsControls() Connecting to SSID: %s, password: %s", wSsid.c_str(), wPassw.c_str());

            Settings.SetValue<String>(ConfigNS::mKeyWifiSSID, wSsid.c_str());
            Settings.SetValue<String>(ConfigNS::mKeyWifiPassword, wPassw.c_str());

            /* Send message to WiFi manager to connect */
            wMessage.mSource = MessageNS::tAddress::WEB_MANAGER;
            wMessage.mDestination = MessageNS::tAddress::WIFI_MANAGER;
            wMessage.mId = MessageNS::tMessageId::CMD_WIFI_CONNECT;

            SendMessage(wMessage);
        }

        return;
    }
    else if (apControl->GetId() == mWebUIControlID.mWifiScanButton)
    {
        /* Scan WiFi networks button pressed */
        if (aType == B_UP)
        {
            LOG(LOG_DEBUG, "WebSite::HandleWiFiSettingsControls() WiFi scan button pressed");

            ESPUI.setEnabled(mWebUIControlID.mWifiScanButton, false);
            ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton, false);

            ESPUI.jsonReload();

            /* Send message to WiFi manager to start scan */
            wMessage.mSource = MessageNS::tAddress::WEB_MANAGER;
            wMessage.mDestination = MessageNS::tAddress::WIFI_MANAGER;
            wMessage.mId = MessageNS::tMessageId::CMD_WIFI_START_SCAN;
            
            SendMessage(wMessage);
        }

        return;
    }
    else
    {
        LOG(LOG_ERROR, "WebSite::HandleControl() Unknown control ID %04X", apControl->GetId());
        return;
    }

    /* Create message */
    wMessage.mSource = MessageNS::tAddress::WEB_MANAGER;
    wMessage.mDestination = MessageNS::tAddress::WEB_MANAGER;

    wMessage.mId = MessageNS::tMessageId::MSG_EVENT_SETTINGS_CHANGED;

    /* Send message */
    SendMessage(wMessage);
}

Control::ControlId_t WebSite::AddTextControl(const char* apTitle, Control::ControlId_t aParent, const String& aElementStyle, const String& arValue)
{
    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Text, apTitle, arValue, Control::Color::Dark, aParent, WebSite::ControlCallback);

    ESPUI.setInputType(wControlId, "text");

    ESPUI.setElementStyle(wControlId, aElementStyle.c_str());

   return wControlId;
}

Control::ControlId_t WebSite::AddTextControl(const char* apLabel, Control::ControlId_t aParent, const String& aElementStyle,
        SettingsNS::tKey aSettingsKey, const String& aDefaultText)
{
    String wValue = aDefaultText;

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wValue = Settings.GetValue<String>(aSettingsKey, aDefaultText);
    }

    Control::ControlId_t wControlId = AddTextControl(apLabel, aParent, aElementStyle, wValue);

    LOG(LOG_DEBUG, "WebSite::AddTextInput() ControlId %04X, param 0x%08X, value %s",
        wControlId, aSettingsKey, wValue.c_str());

    return wControlId;
}


Control::ControlId_t WebSite::AddColorControl(const char* apLabel, Control::ControlId_t aParent, const String& aElementStyle,
        SettingsNS::tKey aSettingsKey, const uint32_t aDefaultColor)
{
    uint32_t wColorParam = 0;
    char wHexColor[10] = {0};

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wColorParam = Settings.GetValue<uint32_t>(aSettingsKey, aDefaultColor);
        sprintf(wHexColor, "#%06X", (wColorParam & 0x00FFFFFF));
    }

    Control::ControlId_t wControlId = AddTextControl(apLabel, aParent, aElementStyle, String(wHexColor));
    ESPUI.setInputType(wControlId, "color");

    LOG(LOG_DEBUG, "WebSite::AddColorInput() Control %04X, param 0x%08X, color %s",
        wControlId, wColorParam, String(wHexColor).c_str());

    return wControlId;

}

Control::ControlId_t WebSite::AddTimeControl(const char* apLabel, Control::ControlId_t aParent, const String& aElementStyle,
        SettingsNS::tKey aSettingsKey, const uint32_t aDefaultTime)
{
    char wTimeStr[6] = {0};  // Buffer for time string in format HH:MM

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        uint32_t wTimeInt = Settings.GetValue<uint32_t>(aSettingsKey, aDefaultTime);
        DateTimeNS::tDateTime wDateTime = DateTimeNS::DwordToDateTime(wTimeInt);

        /* Convert time to string format HH:MM */
        sprintf(wTimeStr, "%02u:%02u", wDateTime.mTime.mHour, wDateTime.mTime.mMinute);
    }

    Control::ControlId_t wControlId = AddTextControl(apLabel, aParent, aElementStyle, String(wTimeStr));
    ESPUI.setInputType(wControlId, "time");

    LOG(LOG_DEBUG, "WebSite::AddTimeInput() Control %04X, time %s", wControlId, String(wTimeStr).c_str());

    return wControlId;
}

Control::ControlId_t WebSite::AddPasswordControl(const char* apLabel, Control::ControlId_t aParent, const String& aElementStyle)
{
    Control::ControlId_t wControlId = AddTextControl(apLabel, aParent, aElementStyle, "");
    ESPUI.setInputType(wControlId, "password");

    LOG(LOG_DEBUG, "WebSite::AddPasswordControl() Control %04X", wControlId);

    return wControlId;
}

Control::ControlId_t WebSite::AddSwitcherControl(const char* apLabel, Control::ControlId_t aParent, const String& aElementStyle,
        SettingsNS::tKey aSettingsKey, const bool aDefaultState)
{
    bool wState = aDefaultState;

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wState = Settings.GetValue<bool>(aSettingsKey, aDefaultState);
    }

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Switcher, apLabel, wState, Control::Color::Dark, Control::noParent, WebSite::ControlCallback);

    LOG(LOG_DEBUG, "WebSite::AddSwitcherControl() Control %04X, default state %s",
        wControlId, aDefaultState ? "ON" : "OFF");

    return wControlId;
}

Control::ControlId_t WebSite::AddSelectControl(const char* apLabel, Control::ControlId_t aParent, const String& aElementStyle,
        const char* const* apItems, uint8_t aItemsCount,
        SettingsNS::tKey aSettingsKey, const uint8_t aDefaultOption)
{
    uint8_t wSelectedOption = 0;

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Select, apLabel, "", Control::Color::Dark, Control::noParent, WebSite::ControlCallback);

    if (apItems != nullptr)
    {
        for (uint8_t wI = 0; wI < aItemsCount; wI++)
        {
            ESPUI.addControl(Control::Type::Option, apItems[wI], String(wI), Control::Color::None, wControlId);
        }
    }

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wSelectedOption = Settings.GetValue<uint8_t>(aSettingsKey, aDefaultOption);
    }

    ESPUI.updateSelect(wControlId, wSelectedOption);

    return wControlId;
}

Control::ControlId_t WebSite::AddPercentageSliderControl(const char* apLabel, Control::ControlId_t aParent, const String& aElementStyle,
        SettingsNS::tKey aSettingsKey, const uint8_t aDefaultValue)
{
    uint8_t wValue = aDefaultValue;

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wValue = Settings.GetValue<uint8_t>(aSettingsKey, aDefaultValue);
    }

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Slider, apLabel, wValue, Control::Color::Dark, Control::noParent, WebSite::ControlCallback);
    ESPUI.addControl(Control::Type::Min, "", String(  0), Control::Color::None, wControlId);
    ESPUI.addControl(Control::Type::Max, "", String(100), Control::Color::None, wControlId);

    LOG(LOG_DEBUG, "WebSite::AddPercentageSliderControl() Control %04X, value %d", wControlId, wValue);

    return wControlId;
}

Control::ControlId_t WebSite::AddButtonControl(const char* apLabel, const String& arValue, Control::ControlId_t aParent, const String& aElementStyle)
{
    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Button, apLabel, arValue, Control::Color::Dark, aParent, WebSite::ControlCallback);

    LOG(LOG_DEBUG, "WebSite::AddButtonControl() Control %04X", wControlId);

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

void WebSite::UpdateWiFiSettingsControls(bool aForceUpdate)
{
    /* Snapshot to avoid race condition with WiFi event handler (different task context) */    
    mLocalSsidList = ConfigNS::mSSSIDList;

    LOG(LOG_DEBUG, "WebSite::UpdateWiFiSettingsControls() Force update %d", aForceUpdate);

    /* Remove all existing options from the select control */
    for (const auto& controlId : mWebUIControlID.mWifiSSIDList)
    {
        ESPUI.removeControl(controlId);
    }
    mWebUIControlID.mWifiSSIDList.clear();

    /* Add new options based on the scanned SSIDs */
    for (size_t wI = 0; wI < mLocalSsidList.size(); wI++)
    {
        Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Option, mLocalSsidList[wI].mSsid, String(wI), Control::Color::None, mWebUIControlID.mWifiSSIDs);
        mWebUIControlID.mWifiSSIDList.push_back(wControlId);
    }

    /* Check if captive portal is enabled */
    if (ESPUI.captivePortal)
    {
        /* Offline Mode */

        /* Select the first available SSID in the select control or an empty string if no SSIDs are available */
        if (!mLocalSsidList.empty())
        {
            ESPUI.updateSelect(mWebUIControlID.mWifiSSIDs, 0);
        }
        else
        {
            ESPUI.updateSelect(mWebUIControlID.mWifiSSIDs, 0);
            LOG(LOG_DEBUG, "WebSite::UpdateWiFiSettingsControls() No SSIDs available, cleared selected SSID.");
        }

        /* Clear password field */
        ESPUI.updateText(mWebUIControlID.mWifiPassword, "");
    }
    else
    {
        /* Normal (online) Mode */

        /* Select the SSID and password from settings in the select control */
        String wSelectedSsid = Settings.GetValue<String>(ConfigNS::mKeyWifiSSID, "");
        String wSelectedPass = Settings.GetValue<String>(ConfigNS::mKeyWifiPassword, "");

        int wSelectedIndex   = -1;

        /* Find the index of the selected SSID in the scanned list */
        for (size_t wI = 0; wI < mLocalSsidList.size(); wI++)
        {
            if (strcmp(mLocalSsidList[wI].mSsid, wSelectedSsid.c_str()) == 0)
            {
                wSelectedIndex = static_cast<int>(wI);
                break;
            }
        }

        /* Selected SSID not found in the scanned list */
        if (wSelectedIndex == -1)
        {
            ConfigNS::tSSIDEntry wNewEntry;
            strncpy(wNewEntry.mSsid, wSelectedSsid.c_str(), sizeof(wNewEntry.mSsid) - 1);
            wNewEntry.mSsid[sizeof(wNewEntry.mSsid) - 1] = '\0';
            wNewEntry.mRssi      = 0;
            wNewEntry.mEncrypted = (wSelectedPass.length() > 0);
            mLocalSsidList.push_back(wNewEntry);

            wSelectedIndex = static_cast<int>(mLocalSsidList.size() - 1);

            Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Option, mLocalSsidList[wSelectedIndex].mSsid, String(wSelectedIndex), Control::Color::None, mWebUIControlID.mWifiSSIDs);
            mWebUIControlID.mWifiSSIDList.push_back(wControlId);

            /* LOG */
            LOG(LOG_DEBUG, "WebSite::UpdateWiFiSettingsControls() Selected SSID not found, added and selected SSID: %s (Control ID: %04X)", wSelectedSsid.c_str(), wControlId);
        }

        /* Update the selected SSID in the select control */
        ESPUI.updateSelect(mWebUIControlID.mWifiSSIDs, wSelectedIndex);
        /* Set password field */
        ESPUI.updateText(mWebUIControlID.mWifiPassword, wSelectedPass);
    }

    ESPUI.setEnabled(mWebUIControlID.mWifiScanButton, true);
    ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton, false);

    if (aForceUpdate)
    {
        ESPUI.jsonReload();
    }
}

void WebSite::HandleColorControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    /* Retreive new color value */
    std::string wColorStr = aControl->getValue().c_str();
    uint32_t  wColorValue = std::stoi(wColorStr.substr(1), nullptr, 16); // skip '#'

    LOG(LOG_DEBUG, "WebSite::HandleColorControl() Control %04X, new color %s (0x%08X)",
            aControl->GetId(), wColorStr.c_str(), wColorValue);

    /* Store new color value in settings */
    Settings.SetValue<uint32_t>(aSettingsKey, wColorValue);
    /* Update displayed value */
    ESPUI.updateText(aControl->GetId(), aControl->getValue());
}

void WebSite::HandleSwitcherControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    /* Retreive new switcher state */
    bool wState = (aType == S_ACTIVE) ? true : false;

    LOG(LOG_DEBUG, "WebSite::HandleSwitcherControl() Control %04X, new state %d",
            aControl->GetId(), wState);

    /* Store new state in settings */
    Settings.SetValue<bool>(aSettingsKey, wState);
}

void WebSite::HandleSelectControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    /* Retreive new selected option */
    uint8_t wSelectedOption = aControl->getValueInt();

    LOG(LOG_DEBUG, "WebSite::HandleSelectControl() control %04X, type %d, selection %d",
            aControl->GetId(), aType, wSelectedOption);

    /* Store new selected option in settings */
    Settings.SetValue<uint8_t>(aSettingsKey, wSelectedOption);
}

void WebSite::HandlePercentageSliderControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    /* Retreive new slider value */
    uint8_t wValue = aControl->getValueInt();

    LOG(LOG_DEBUG, "WebSite::HandlePercentageSliderControl() Control %04X, new value %d",
            aControl->GetId(), wValue);

    /* Store new slider value in settings */
    Settings.SetValue<uint8_t>(aSettingsKey, wValue);
}

void WebSite::HandleTimerControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    /* Retreive new time value in format HH:MM */
    std::string wTimeStr = aControl->getValue().c_str();

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

void WebSite::ControlCallback(BasicControl* apSender, int aType , void* apParam)
{
    if (mpWebSiteInstance)
    {
        mpWebSiteInstance->HandleControl(apSender, aType, apParam);
    }
}
