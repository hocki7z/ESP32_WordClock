/*
 * WebSite.cpp
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#include <ESPUI.h>
#include <ESPmDNS.h>
#include <sdkconfig.h>
#include "WordClockTlsCredentials.h"

#if !defined(CONFIG_ESP_HTTPS_SERVER_ENABLE) || !CONFIG_ESP_HTTPS_SERVER_ENABLE \
    || !defined(CONFIG_HTTPD_WS_SUPPORT) || !CONFIG_HTTPD_WS_SUPPORT
#error "WordClock HTTPS requires a framework built with HTTPS server and WebSocket support"
#endif

#include "Logger.h"
#include "Configuration.h"
#include "DateTime.h"
#include "Serialize.h"
#include "BitMatrix.h"
#include "DisplayLayout.h"
#include "Settings.hpp"

#include "WebSite.h"


/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)

/* CSS styles for the web UI controls */
#define CSS_STYLE_NONE                  ""
#define CSS_STYLE_GROUP                 "background-color: unset; width: 100%; text-align: center;"
#define CSS_STYLE_SEPARATOR             "background-color: unset; width: 100%;" // min-height:2px
#define CSS_STYLE_LABEL                 "background-color: unset; width: 100%; text-align: left; padding: 0;"
#define CSS_STYLE_INPUT                 "color: black; width: calc(100% - 8px); height: 26px; margin-left: 8px; box-sizing: border-box;"
#define CSS_STYLE_SLIDER                "margin-left: 8px; width: calc(100% - 53px); box-sizing: border-box;"
#define CSS_STYLE_PREVIEW               "background-color: #000000; padding: 0; width: 100%; text-align: center;"
#define CSS_STYLE_SWITCH                "margin-bottom: 10px; align-items: center; text-align: right; vertical-align: middle;" // float: right; margin: 0 8px 10px 0;
#define CSS_STYLE_SWITCH_LABEL          "background-color: unset; width: calc(100% - 68px); text-align: left; padding: 0; vertical-align: middle;"
#define CSS_STYLE_BUTTON                "width: 100%" //; font-size:1.6rem;min-height:4rem;padding:1rem;margin:3px;border:2px solid #404040;border-radius:5px;"


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

    /*
     * Initialize WEB UI but do not start it yet
     */

    /* ESPUI log mode - turn on/off verbose debugging */
    ESPUI.setVerbosity((LOG_LEVEL == LOG_VERBOSE) ? Verbosity::Verbose : Verbosity::Quiet);

    /* Both PEM strings are copied before HTTP/AP or HTTPS/STA can start. */
    if (!ESPUI.setCertificate(WordClockTls::Certificate, WordClockTls::PrivateKey))
    {
        LOG(LOG_ERROR, "Cannot configure HTTPS certificate; HTTPS will not fall back to HTTP");
    }

	/* Make sliders continually report their position as they are being dragged. */
	ESPUI.sliderContinuous = true;

    /**
     * Group Overview
     */
    mWebUIControlID.mOverViewGroup = AddGroupHelper("Overview", Control::noParent, Control::Color::Wetasphalt);

    /* Live preview of the word-clock LED matrix */
    mWebUIControlID.mDisplayPreview = AddLabelControl("", mWebUIControlID.mOverViewGroup, CSS_STYLE_PREVIEW,
        "Here will be a live preview of the word-clock LED matrix");    // Please wait until the first update is received from the device

    /**
     * Group Wordclock settings
     */
    mWebUIControlID.mSettingsWordclockGroup = AddGroupHelper("Wordclock", Control::noParent, Control::Color::Wetasphalt);

    /* Clock mode */
    AddLabelControl("", mWebUIControlID.mSettingsWordclockGroup, CSS_STYLE_LABEL, "Clock mode");
    mWebUIControlID.mDisplayClockMode = AddSelectControl("", mWebUIControlID.mSettingsWordclockGroup, CSS_STYLE_INPUT,
            ConfigNS::mcClockModeItems, ConfigNS::mcClockModeItemsCount,
            ConfigNS::mKeyDisplayClockMode, ConfigNS::mDefaultDisplayClockMode);

    /* Switcher for 'IT IS' words */
    AddLabelControl("", mWebUIControlID.mSettingsWordclockGroup, CSS_STYLE_SWITCH_LABEL, "Show 'IT IS'");
    mWebUIControlID.mDisplayClockItIs = AddSwitcherControl("", mWebUIControlID.mSettingsWordclockGroup, CSS_STYLE_SWITCH,
            ConfigNS::mKeyDisplayClockItIs, ConfigNS::mDefaultDisplayClockItIs);

    /* Switch for single minutes */
    AddLabelControl("", mWebUIControlID.mSettingsWordclockGroup, CSS_STYLE_SWITCH_LABEL, "Show single minutes");
    mWebUIControlID.mDisplayClockSingleMinutes = AddSwitcherControl("", mWebUIControlID.mSettingsWordclockGroup, CSS_STYLE_SWITCH,
            ConfigNS::mKeyDisplayClockSingleMins, ConfigNS::mDefaultDisplayClockSingleMins);


    /**
     * Group LED settings
     */
    mWebUIControlID.mSettingsLedGroup = AddGroupHelper("LED", Control::noParent, Control::Color::Wetasphalt);

    /* Time color */
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_LABEL, "Time color");
    mWebUIControlID.mDisplayColorTime = AddColorControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_INPUT,
            ConfigNS::mKeyDisplayColorTime, ConfigNS::mDefaultDisplayColorTime);

    /* Background color */
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_LABEL, "Background color");
    mWebUIControlID.mDisplayColorBackground = AddColorControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_INPUT,
            ConfigNS::mKeyDisplayColorBkgd, ConfigNS::mDefaultDisplayColorBkgd);

    /* Slider for LED brightness selection */
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_LABEL, "LED brightness");
    mWebUIControlID.mDisplayLedBrightness = AddPercentageSliderControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_SLIDER,
            ConfigNS::mKeyDisplayLedBrightness, ConfigNS::mDefaultDisplayLedBrightness);

    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_SEPARATOR, "");

    /* Switcher for day/night mode activation */
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_SWITCH_LABEL, "Use day/night mode");

    mWebUIControlID.mDisplayUseNightMode = AddSwitcherControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_SWITCH,
            ConfigNS::mKeyDisplayUseNightMode, ConfigNS::mDefaultDisplayUseNightMode);
    /* Slider for night brightness selection */
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_LABEL, "Night mode brightness");
    mWebUIControlID.mDisplayBrightnessNightMode = AddPercentageSliderControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_SLIDER,
            ConfigNS::mKeyDisplayBrightnessNightMode, ConfigNS::mDefaultDisplayBrightnessNightMode);
    /* Time input for night mode start */
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_LABEL, "Night mode start time");
    mWebUIControlID.mDisplayNightModeStartTime = AddTimeControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_INPUT,
            ConfigNS::mKeyDisplayNightModeStartTime, ConfigNS::mDefaultDisplayNightModeStartTime);
    /* Time input for night mode end */
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_LABEL, "Night mode end time");
    mWebUIControlID.mDisplayNightModeEndTime = AddTimeControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_INPUT,
            ConfigNS::mKeyDisplayNightModeEndTime, ConfigNS::mDefaultDisplayNightModeEndTime);


    /**
     * Group Time settings
     */
    mWebUIControlID.mSettingsTimeGroup = AddGroupHelper("Time settings", Control::noParent, Control::Color::Wetasphalt);

    /* NTP server selection */
    AddLabelControl("", mWebUIControlID.mSettingsTimeGroup, CSS_STYLE_LABEL, "NTP server");
    mWebUIControlID.mDatetimeNtpServer = AddSelectControl("", mWebUIControlID.mSettingsTimeGroup, CSS_STYLE_INPUT,
            ConfigNS::mcNtpServerItems, ConfigNS::mcNtpServerItemsCount,
            ConfigNS::mKeyNtpServer, ConfigNS::mDefaultNtpServer);
    /* Timezone selection */
    AddLabelControl("", mWebUIControlID.mSettingsTimeGroup, CSS_STYLE_LABEL, "Time zone");
    mWebUIControlID.mDatetimeTimeZone = AddSelectControl("", mWebUIControlID.mSettingsTimeGroup, CSS_STYLE_INPUT,
            ConfigNS::mcTimezoneNames, ConfigNS::mcTimezoneItemsCount,
            ConfigNS::mKeyTimeZone, ConfigNS::mDefaultTimeZone);


    /**
     * Group WiFi settings
     */
    mWebUIControlID.mSettingsWiFiGroup = AddGroupHelper("WiFi Access Point Credentials", Control::noParent, Control::Color::Wetasphalt);

    /* WiFi settings controls here (e.g., SSID, password, etc.) */
    AddLabelControl("", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_LABEL, "SSID");
    mWebUIControlID.mWifiSSIDs = AddSelectControl("", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_INPUT,
        nullptr, 0, ConfigNS::mInvalidKey, 0, ControlHandler::WifiSsidSelect);

    AddLabelControl("", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_LABEL, "Password");
    mWebUIControlID.mWifiPassword = AddPasswordControl("", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_INPUT,
        ConfigNS::mInvalidKey, ControlHandler::WifiPassword);

    AddLabelControl("", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_SWITCH_LABEL, "Show/Hide Password");
    mWebUIControlID.mWifiPasswordShowHide = AddSwitcherControl("", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_SWITCH,
        ConfigNS::mInvalidKey, false, ControlHandler::WifiPasswordShowHide);

    /* Add buttons for scanning WiFi networks and connecting to the selected network */
    mWebUIControlID.mWifiConnectButton = AddButtonControl("", "Save & Connect", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_BUTTON,
        ControlHandler::WifiConnectButton);

    mWebUIControlID.mWifiScanButton = AddButtonControl("", "Search for WiFi", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_BUTTON,
        ControlHandler::WifiScanButton);
    mWebUIControlID.mWifiForgetButton = AddButtonControl("", "Forget network", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_BUTTON,
        ControlHandler::WifiForgetButton);

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

            /* Update WiFi settings controls */
            UpdateWiFiSettingsControls();

            /* Advertise the certificate's stable hostname instead of a DHCP IP. */
            MDNS.end();
            if (MDNS.begin(WordClockTls::Hostname))
            {
                MDNS.addService("https", "tcp", 443);
            }
            else
            {
                LOG(LOG_ERROR, "Cannot start mDNS; HTTPS remains accessible by IP with a name warning");
            }

            /* Normal operation is TLS only. No silent plaintext fallback. */
            ESPUI.beginHTTPS("Wordclock");
            if (ESPUI.WebServer() && ESPUI.WebServer()->isRunning() && ESPUI.WebServer()->getPort() == 443)
            {
                LOG(LOG_INFO, "Web UI: https://%s.local/", WordClockTls::Hostname);
            }
            break;

        case MessageNS::tMessageId::MSG_EVENT_WIFI_AP_STARTED:
            /* Access point started, start WEB server */
            LOG(LOG_DEBUG, "WebSite::ProcessIncomingMessage() Start web server in AP mode");

            /* Offline Mode - enable captive portal */
            ESPUI.captivePortal = true;
            MDNS.end();

            /* Update WiFi settings controls */
            UpdateWiFiSettingsControls();

            /* AP provisioning deliberately remains HTTP for captive discovery. */
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

        case MessageNS::tMessageId::CMD_WEBSITE_HANDLE_WIFI_CONTROLS:
        {
            /* Received message for handling WiFi controls */
            if (arMessage.mPayloadLength == 5)
            {
                ControlHandler wHandler;
                uint32_t wTypeRaw;
                int wType;

                /* Retrieve ControlHandler from the payloads byte 0 */
                wHandler = static_cast<ControlHandler>(arMessage.mPayload[0]);

                /* Deserialize the type from the payloads bytes 2..5 */
                SerializeNS::DeserializeData(arMessage.mPayload, &wTypeRaw, 1);
                wType = static_cast<int32_t>(wTypeRaw);

                /* Handle WiFi controls */
                HandleWiFiControls(wHandler, wType);
            }
            else
            {
                LOG(LOG_ERROR, "WebSite::ProcessIncomingMessage() Invalid payload length for CMD_WEBSITE_HANDLE_WIFI_CONTROLS: %d", arMessage.mPayloadLength);
            }
        }
            break;

        case MessageNS::tMessageId::MSG_EVENT_DISPLAY_FRAME_READY:
            UpdateDisplayPreview(arMessage);
            break;

        default:
            // do nothing
            break;
    }
}

void WebSite::HandleControl(BasicControl* apControl, int aType, void* apParam)
{
    ControlHandler       wHandler = ControlHandler::None;
    SettingsNS::tKey wSettingsKey = ConfigNS::mInvalidKey;

    /* Message to be sent if control changes */
    MessageNS::Message wMessage;
    /* Flag to indicate whether to send a message or not */
    bool wSendMessage = false;  // default - false

    /* Prepare message */
    wMessage.mSource = MessageNS::tAddress::WEB_MANAGER;
    wMessage.mDestination = MessageNS::tAddress::WEB_MANAGER;
    wMessage.mId = MessageNS::tMessageId::MSG_EVENT_SETTINGS_CHANGED;

    /* Recover the handler tag + settings key handed to ESPUI at control creation time */
    if (apParam != nullptr)
    {
        const ControlParam* wParam = static_cast<const ControlParam*>(apParam);

        wHandler     = wParam->mHandler;
        wSettingsKey = wParam->mSettingsKey;
    }

    /* Handle control based on its handler tag */
    switch (wHandler)
    {
        case ControlHandler::Text:
            wSendMessage = HandleTextControl(apControl, aType, wSettingsKey);
            break;

        case ControlHandler::Color:
            wSendMessage = HandleColorControl(apControl, aType, wSettingsKey);
            break;

        case ControlHandler::Timer:
            wSendMessage = HandleTimerControl(apControl, aType, wSettingsKey);
            break;

        case ControlHandler::Switcher:
            wSendMessage = HandleSwitcherControl(apControl, aType, wSettingsKey);
            break;

        case ControlHandler::Select:
            wSendMessage = HandleSelectControl(apControl, aType, wSettingsKey);
            break;

        case ControlHandler::Slider:
            wSendMessage = HandleSliderControl(apControl, aType, wSettingsKey);
            break;

        default:
            /* LOG */
            LOG(LOG_ERROR, "WebSite::HandleControl() Unknown control handler for control ID %04X", apControl->GetId());
            break;
    }

    /* Send message if needed */
    if (wSendMessage)
    {
        SendMessage(wMessage);
    }
}

void WebSite::HandleWiFiControls(ControlHandler aHandler, int aType)
{
    /* Message to be sent if control changes */
    MessageNS::Message wMessage;
    /* Flag to indicate whether to send a message or not */
    bool wSendMessage = false;  // default - false

    /* Prepare message */
    wMessage.mSource = MessageNS::tAddress::WEB_MANAGER;
    wMessage.mDestination = MessageNS::tAddress::WIFI_MANAGER;

    /* Handle control based on its handler tag */
    switch (aHandler)
    {
        case ControlHandler::WifiSsidSelect:
        case ControlHandler::WifiPassword:
        {
            bool wIsSsidValid     = false;
            bool wIsPasswordValid = false;
            tSSIDEntry wSelectedSsid{};

            /* Get current WiFi SSID selection */
            const long wSsidIndex = ESPUI.getControl(mWebUIControlID.mWifiSSIDs)->getValueInt();
            /* Get current WiFi password */
            String wPassw = ESPUI.getControl(mWebUIControlID.mWifiPassword)->getValue().c_str();

            wIsSsidValid = (wSsidIndex >= 0) &&
                (static_cast<size_t>(wSsidIndex) < mcMaxSsidCount) &&
                mSSIDList.Get(static_cast<size_t>(wSsidIndex), wSelectedSsid) &&    // Copy the selected entry
                wSelectedSsid.IsValid();

            if (wIsSsidValid)
            {
                /* Allows any valid SSID in captive-portal mode;
                 * otherwise, excludes index 0 (the current network) */
                wIsSsidValid = ESPUI.captivePortal || (wSsidIndex > 0);

                /* Check password validity: it is non-empty and the selected network is encrypted */
                wIsPasswordValid = ((wPassw.length() > 7) && wSelectedSsid.mEncrypted);    // Technical minimum length: 8 characters
            }

            ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton, (wIsSsidValid && wIsPasswordValid));

            /* Wait a bit to avoid issues with the UI update */
            delay(100);
        }
            break;

        case ControlHandler::WifiPasswordShowHide:
        {
            /* WiFi password show/hide switcher changed */
            bool wState = (aType == S_ACTIVE) ? true : false;

            /* Set the input type of the password field based on the switcher state */
            ESPUI.setInputType(mWebUIControlID.mWifiPassword, wState ? "text" : "password");

            /* Wait a bit to avoid issues with the UI update */
            delay(100);
        }
            break;

        case ControlHandler::WifiConnectButton:
        {
            /* Connect to selected network button pressed */
            if (aType == B_UP)
            {
                LOG(LOG_DEBUG, "WebSite::HandleControl() WiFi connect button pressed");

                /* Get selected SSID and password from the controls */
                const long wSsidIndex = ESPUI.getControl(mWebUIControlID.mWifiSSIDs)->getValueInt();
                /* Copy the selected entry through the synchronized Configuration API */
                tSSIDEntry wSelectedSsid{};
                mSSIDList.Get(static_cast<size_t>(wSsidIndex), wSelectedSsid);

                String wSsid  = wSelectedSsid.mSsid;
                String wPassw = ESPUI.getControl(mWebUIControlID.mWifiPassword)->getValue().c_str();

                LOG(LOG_DEBUG, "WebSite::HandleControl() Connecting to SSID: %s, password: %s", wSsid.c_str(), wPassw.c_str());

                /* Store SSID and password in settings */
                Settings.SetValue<String>(ConfigNS::mKeyWifiSSID, wSsid.c_str());
                Settings.SetValue<String>(ConfigNS::mKeyWifiPassword, wPassw.c_str());

                /* Message to WiFi manager with connect command */
                wMessage.mId = MessageNS::tMessageId::CMD_WIFI_CONNECT;
                wSendMessage = true;
            }
        }
            break;

        case ControlHandler::WifiScanButton:
        {
            /* Scan WiFi networks button pressed */
            if (aType == B_UP)
            {
                LOG(LOG_DEBUG, "WebSite::HandleControl() WiFi scan button pressed");

                /* Update scan button label with ongoing status */
                ESPUI.updateLabel(mWebUIControlID.mWifiScanButton, "WiFi scan ongoing, please wait...");

                /* Disable WiFi settings controls during scanning */
                ESPUI.setEnabled(mWebUIControlID.mWifiSSIDs,            false);
                ESPUI.setEnabled(mWebUIControlID.mWifiPassword,         false);
                ESPUI.setEnabled(mWebUIControlID.mWifiPasswordShowHide, false);
                ESPUI.setEnabled(mWebUIControlID.mWifiForgetButton,     false);
                ESPUI.setEnabled(mWebUIControlID.mWifiScanButton,       false);
                ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton,    false);

                /* Wait a bit before sending the scan command to avoid issues with the UI update */
                delay(100);

                /* Message to WiFi manager start scan command */
                wMessage.mId = MessageNS::tMessageId::CMD_WIFI_START_SCAN;
                wSendMessage = true;
            }
        }
            break;

        case ControlHandler::WifiForgetButton:
        {
            /* Forget WiFi network button pressed */
            if (aType == B_UP)
            {
                LOG(LOG_DEBUG, "WebSite::HandleControl() WiFi forget button pressed");

                /* Remove SSID and password from settings */
                Settings.RemoveKey(ConfigNS::mKeyWifiSSID);
                Settings.RemoveKey(ConfigNS::mKeyWifiPassword);

                /* Message to WiFi manager with connect command */
                wMessage.mId = MessageNS::tMessageId::CMD_WIFI_CONNECT;
                wSendMessage = true;
            }
        }
            break;

        default:
            /* LOG */
            LOG(LOG_ERROR, "WebSite::HandleWiFiControls() Unknown control handler %d", aHandler);
            break;
    }

    /* Send message if needed */
    if (wSendMessage)
    {
        SendMessage(wMessage);
    }
}

/**
 * @brief Handle text control changes and update settings
 * @param aControl Pointer to the BasicControl that triggered the change
 * @param aType The type of the control event (e.g., S_ACTIVE, S_INACTIVE)
 * @param aSettingsKey The settings key associated with the control
 *
 * @return True if the settings were updated successfully, false otherwise
 */
bool WebSite::HandleTextControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    bool wRetValue = false;

    LOG(LOG_DEBUG, "WebSite::HandleTextControl() Control %04X, new value %s",
        aControl->GetId(), aControl->getValue().c_str());

    /* Store new text value in settings if setting key is valid */
    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        /* Set return value based on whether the settings were updated successfully */
        wRetValue = Settings.SetValue<String>(aSettingsKey, aControl->getValue());
    }

    return wRetValue;
}

/**
 * @brief Handle color control changes and update settings
 * @param aControl Pointer to the BasicControl that triggered the change
 * @param aType The type of the control event (e.g., S_ACTIVE, S_INACTIVE)
 * @param aSettingsKey The settings key associated with the control
 *
 * @return True if the settings were updated successfully, false otherwise
 */
bool WebSite::HandleColorControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    bool wRetValue = false;

    /* Retreive new color value */
    std::string wColorStr = aControl->getValue().c_str();
    uint32_t  wColorValue = std::stoi(wColorStr.substr(1), nullptr, 16); // skip '#'

    /* Update displayed value */
    ESPUI.updateText(aControl->GetId(), aControl->getValue());

    LOG(LOG_DEBUG, "WebSite::HandleColorControl() Control %04X, new color %s (0x%08X)",
            aControl->GetId(), wColorStr.c_str(), wColorValue);

    /* Store new text value in settings if setting key is valid */
    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        /* Set return value based on whether the settings were updated successfully */
        wRetValue = Settings.SetValue<uint32_t>(aSettingsKey, wColorValue);

    }

    return wRetValue;
}

/**
 * @brief Handle switcher control changes and update settings
 * @param aControl Pointer to the BasicControl that triggered the change
 * @param aType The type of the control event (e.g., S_ACTIVE, S_INACTIVE)
 * @param aSettingsKey The settings key associated with the control
 *
 * @return True if the settings were updated successfully, false otherwise
 */
bool WebSite::HandleSwitcherControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    bool wRetValue = false;

    /* Retreive new switcher state */
    bool wState = (aType == S_ACTIVE) ? true : false;

    LOG(LOG_DEBUG, "WebSite::HandleSwitcherControl() Control %04X, new state %d",
            aControl->GetId(), wState);

    /* Store new text value in settings if setting key is valid */
    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        /* Set return value based on whether the settings were updated successfully */
        wRetValue = Settings.SetValue<bool>(aSettingsKey, wState);
    }

    return wRetValue;
}

/**
 * @brief Handle select control changes and update settings
 * @param aControl Pointer to the BasicControl that triggered the change
 * @param aType The type of the control event (e.g., S_ACTIVE, S_INACTIVE)
 * @param aSettingsKey The settings key associated with the control
 *
 * @return True if the settings were updated successfully, false otherwise
 */
bool WebSite::HandleSelectControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    bool wRetValue = false;

    /* Retreive new selected option */
    uint8_t wSelectedOption = aControl->getValueInt();

    LOG(LOG_DEBUG, "WebSite::HandleSelectControl() control %04X, type %d, selection %d",
            aControl->GetId(), aType, wSelectedOption);

    /* Store new selected option in settings if setting key is valid */
    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        /* Set return value based on whether the settings were updated successfully */
        wRetValue = Settings.SetValue<uint8_t>(aSettingsKey, wSelectedOption);
    }

    return wRetValue;
}

/**
 * @brief Handle slider control changes and update settings
 * @param aControl Pointer to the BasicControl that triggered the change
 * @param aType The type of the control event (e.g., S_ACTIVE, S_INACTIVE)
 * @param aSettingsKey The settings key associated with the control
 *
 * @return True if the settings were updated successfully, false otherwise
 */
bool WebSite::HandleSliderControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    bool wRetValue = false;

    /* Retreive new slider value */
    uint8_t wValue = aControl->getValueInt();

    LOG(LOG_DEBUG, "WebSite::HandleSliderControl() Control %04X, new value %d",
            aControl->GetId(), wValue);

    /* Store new slider value in settings if setting key is valid */
    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        /* Set return value based on whether the settings were updated successfully */
        wRetValue = Settings.SetValue<uint8_t>(aSettingsKey, wValue);
    }

    return wRetValue;
}

/**
 * @brief Handle timer control changes and update settings
 * @param aControl Pointer to the BasicControl that triggered the change
 * @param aType The type of the control event (e.g., S_ACTIVE, S_INACTIVE)
 * @param aSettingsKey The settings key associated with the control
 *
 * @return True if the settings were updated successfully, false otherwise
 */
bool WebSite::HandleTimerControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    bool wRetValue = false;

    /* Retreive new time value in format HH:MM */
    std::string wTimeStr = aControl->getValue().c_str();

    uint8_t wHour   = static_cast<uint8_t>(std::stoi(wTimeStr.substr(0, 2)));
    uint8_t wMinute = static_cast<uint8_t>(std::stoi(wTimeStr.substr(3, 2)));

    LOG(LOG_DEBUG, "WebSite::HandleTimerControl() Control %04X, new time %02u:%02u",
            aControl->GetId(), wHour, wMinute);

    /* Store new time value in settings */
    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        DateTimeNS::tDateTime wDateTime;
        wDateTime.mTime.mHour   = wHour;
        wDateTime.mTime.mMinute = wMinute;
        wDateTime.mTime.mSecond = 0;
        wDateTime.mDate.mDay    = 1;
        wDateTime.mDate.mMonth  = 1;
        wDateTime.mDate.mYear   = DateTimeNS::mYearRangeStart;

        uint32_t wTimeDword = DateTimeNS::DateTimeToDword(wDateTime);

        /* Set return value based on whether the settings were updated successfully */
        wRetValue = Settings.SetValue<uint32_t>(aSettingsKey, wTimeDword);
    }

    return wRetValue;
}

/**
 * @brief Helper function to add a group control to the web UI
 * @param apLabel The label for the group control
 * @param aParent The parent control ID (default: no parent)
 * @param aColor The color of the group control (default: None)
 *
 * @return The control ID of the newly added group control
 */
Control::ControlId_t WebSite::AddGroupHelper(const char * apLabel, Control::ControlId_t aParent, Control::Color aColor)
{
	Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Label, apLabel, "", aColor, aParent);
	ESPUI.setElementStyle(wControlId, CSS_STYLE_GROUP);

    return wControlId;
}

/**
 * @brief Helper function to add a label control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param arValue The initial value of the label (default: empty string)
 *
 * @return The control ID of the newly added label control
 */
Control::ControlId_t WebSite::AddLabelControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle, const String& arValue)
{
    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Label, apLabel, arValue, Control::Color::None, aParent);
    ESPUI.setElementStyle(wControlId, aElementStyle);

    return wControlId;
}

/**
 * @brief Helper function to add a text control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param arValue The initial value of the text control (default: empty string)
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aHandler The control handler for the control (default: None)
 *
 * @return The control ID of the newly added text control
 */
Control::ControlId_t WebSite::AddTextControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle, const String& arValue,
        SettingsNS::tKey aSettingsKey, ControlHandler aHandler)
{
    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Text, apLabel, arValue, Control::Color::Dark, aParent,
            WebSite::ControlCallback, new ControlParam{aHandler, aSettingsKey});

    ESPUI.setInputType(wControlId, "text");

    ESPUI.setElementStyle(wControlId, aElementStyle);

   return wControlId;
}

/**
 * @brief Helper function to add a text control to the web UI with a settings key
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aDefaultText The default text value (default: empty string)
 * @param aHandler The control handler for the control (default: Text)
 *
 * @return The control ID of the newly added text control
 */
Control::ControlId_t WebSite::AddTextControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        SettingsNS::tKey aSettingsKey, const String& aDefaultText, ControlHandler aHandler)
{
    String wValue = aDefaultText;

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wValue = Settings.GetValue<String>(aSettingsKey, aDefaultText);
    }

    Control::ControlId_t wControlId = AddTextControl(apLabel, aParent, aElementStyle, wValue, aSettingsKey, aHandler);

    LOG(LOG_DEBUG, "WebSite::AddTextControl() ControlId %04X, param 0x%08X, value %s, handler %d",
        wControlId, aSettingsKey, wValue.c_str(), static_cast<int>(aHandler));

    return wControlId;
}

/**
 * @brief Helper function to add a color control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aDefaultColor The default color value (default: 0x000000)
 * @param aHandler The control handler for the control (default: Color)
 *
 * @return The control ID of the newly added color control
 */
Control::ControlId_t WebSite::AddColorControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        SettingsNS::tKey aSettingsKey, const uint32_t aDefaultColor, ControlHandler aHandler)
{
    uint32_t wColorParam = aDefaultColor;
    char wHexColor[8] = {0};

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wColorParam = Settings.GetValue<uint32_t>(aSettingsKey, aDefaultColor);
    }

    snprintf(wHexColor, sizeof(wHexColor), "#%06lX",
        static_cast<unsigned long>(wColorParam & 0x00FFFFFF));

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Text, apLabel, String(wHexColor), Control::Color::Dark, aParent,
            WebSite::ControlCallback, new ControlParam{aHandler, aSettingsKey});

    ESPUI.setElementStyle(wControlId, aElementStyle);

    LOG(LOG_DEBUG, "WebSite::AddColorControl() Control %04X, param 0x%08X, color %s",
        wControlId, wColorParam, wHexColor);

    return wControlId;
}

/**
 * @brief Helper function to add a time control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aDefaultTime The default time value (default: 0)
 * @param aHandler The control handler for the control (default: Time)
 *
 * @return The control ID of the newly added time control
 */
Control::ControlId_t WebSite::AddTimeControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        SettingsNS::tKey aSettingsKey, const uint32_t aDefaultTime, ControlHandler aHandler)
{
    char wTimeStr[6] = {0};  // Buffer for time string in format HH:MM

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        uint32_t wTimeInt = Settings.GetValue<uint32_t>(aSettingsKey, aDefaultTime);
        DateTimeNS::tDateTime wDateTime = DateTimeNS::DwordToDateTime(wTimeInt);

        /* Convert time to string format HH:MM */
        sprintf(wTimeStr, "%02u:%02u", wDateTime.mTime.mHour, wDateTime.mTime.mMinute);
    }

    Control::ControlId_t wControlId = AddTextControl(apLabel, aParent, aElementStyle, String(wTimeStr), aSettingsKey, aHandler);
    ESPUI.setInputType(wControlId, "time");

    LOG(LOG_DEBUG, "WebSite::AddTimeInput() Control %04X, time %s", wControlId, String(wTimeStr).c_str());

    return wControlId;
}

/**
 * @brief Helper function to add a password control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aHandler The control handler for the control (default: Password)
 *
 * @return The control ID of the newly added password control
 */
Control::ControlId_t WebSite::AddPasswordControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        SettingsNS::tKey aSettingsKey, ControlHandler aHandler)
{
    Control::ControlId_t wControlId = AddTextControl(apLabel, aParent, aElementStyle, emptyString, aSettingsKey, aHandler);
    ESPUI.setInputType(wControlId, "password");

    LOG(LOG_DEBUG, "WebSite::AddPasswordControl() Control %04X", wControlId);

    return wControlId;
}

/**
 * @brief Helper function to add a switcher control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aDefaultState The default state of the switcher (default: false)
 * @param aHandler The control handler for the control (default: Switcher)
 *
 * @return The control ID of the newly added switcher control
 */
Control::ControlId_t WebSite::AddSwitcherControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        SettingsNS::tKey aSettingsKey, const bool aDefaultState, ControlHandler aHandler)
{
    bool wState = aDefaultState;

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wState = Settings.GetValue<bool>(aSettingsKey, aDefaultState);
    }

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Switcher, apLabel, wState, Control::Color::Dark, aParent,
            WebSite::ControlCallback, new ControlParam{aHandler, aSettingsKey});

    ESPUI.setElementStyle(wControlId, aElementStyle);

    LOG(LOG_DEBUG, "WebSite::AddSwitcherControl() Control %04X, default state %s",
        wControlId, aDefaultState ? "ON" : "OFF");

    return wControlId;
}
/**
 * @brief Helper function to add a select control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param apItems The array of items for the select control
 * @param aItemsCount The number of items in the array
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aDefaultOption The default selected option (default: 0)
 * @param aHandler The control handler for the control (default: Select)
 *
 * @return The control ID of the newly added select control
 */
Control::ControlId_t WebSite::AddSelectControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        const char* const* apItems, uint8_t aItemsCount,
        SettingsNS::tKey aSettingsKey, const uint8_t aDefaultOption, ControlHandler aHandler)
{
    uint8_t wSelectedOption = 0;

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Select, apLabel, "", Control::Color::None, aParent,
            WebSite::ControlCallback, new ControlParam{aHandler, aSettingsKey});

    ESPUI.setElementStyle(wControlId, aElementStyle);

    if (apItems != nullptr)
    {
        for (uint8_t wI = 0; wI < aItemsCount; wI++)
        {
            ESPUI.addControl(Control::Type::Option, apItems[wI], static_cast<long>(wI), Control::Color::None, wControlId);
        }
    }

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wSelectedOption = Settings.GetValue<uint8_t>(aSettingsKey, aDefaultOption);
    }

    ESPUI.updateSelect(wControlId, wSelectedOption);

    return wControlId;
}

/**
 * @brief Helper function to add a percentage slider control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aDefaultValue The default value of the slider (default: 50)
 * @param aHandler The control handler for the control (default: Slider)
 *
 * @return The control ID of the newly added percentage slider control
 */
Control::ControlId_t WebSite::AddPercentageSliderControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        SettingsNS::tKey aSettingsKey, const uint8_t aDefaultValue, ControlHandler aHandler)
{
    uint8_t wValue = aDefaultValue;

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wValue = Settings.GetValue<uint8_t>(aSettingsKey, aDefaultValue);
    }

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Slider, apLabel, wValue, Control::Color::Dark, aParent,
            WebSite::ControlCallback, new ControlParam{aHandler, aSettingsKey});

    ESPUI.setElementStyle(wControlId, aElementStyle);

    ESPUI.addControl(Control::Type::Min, "", String(  0), Control::Color::None, wControlId);
    ESPUI.addControl(Control::Type::Max, "", String(100), Control::Color::None, wControlId);

    LOG(LOG_DEBUG, "WebSite::AddPercentageSliderControl() Control %04X, value %d", wControlId, wValue);

    return wControlId;
}

/**
 * @brief Helper function to add a button control to the web UI
 * @param apLabel The label for the control
 * @param arValue The value of the button
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aHandler The control handler for the control (default: None)
 *
 * @return The control ID of the newly added button control
 */
Control::ControlId_t WebSite::AddButtonControl(const char* apLabel, const String& arValue, Control::ControlId_t aParent, const char* aElementStyle,
        ControlHandler aHandler)
{
    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Button, apLabel, arValue, Control::Color::None, aParent,
            WebSite::ControlCallback, new ControlParam{aHandler, ConfigNS::mInvalidKey});

    ESPUI.setElementStyle(wControlId, aElementStyle);

    LOG(LOG_DEBUG, "WebSite::AddButtonControl() Control %04X", wControlId);

    return wControlId;
}

/**
 * @brief Build an SVG representation of the display preview
 * @param apDisplayScene Pointer to the display scene
 * @return SVG string representing the display preview
 */
String WebSite::BuildDisplayPreviewSvg(const DisplayScene* apDisplayScene) const
{
    /* Array for cell colors */
    tColor wCellColors[MATRIX_HEIGHT][MATRIX_WIDTH] = {};

    /* Retrieve background color from settings */
    tColor wBackgroundColor = static_cast<tColor>(Settings.GetValue<uint32_t>(
        ConfigNS::mKeyDisplayColorBkgd, ConfigNS::mDefaultDisplayColorBkgd) & COLOR_RGB_MASK);

    /* Check if display scene is available */
    if (apDisplayScene)
    {
        /* Use background color from display scene */
        wBackgroundColor = apDisplayScene->mBackground & COLOR_RGB_MASK;

        /* Fill colors array with background color */
        for (uint8_t wRow = 0; wRow < MATRIX_HEIGHT; ++wRow)
        {
            for (uint8_t wCol = 0; wCol < MATRIX_WIDTH; ++wCol)
            {
                wCellColors[wRow][wCol] = wBackgroundColor;
            }
        }

        /* Lambda to add a word group to the display preview */
        auto wAddGroup = [&wCellColors](const WordGroup& arGroup)
        {
            if (arGroup.IsEnabled())
            {
                tColor wGroupColor = arGroup.mColor & COLOR_RGB_MASK;

                /**
                 * Iterate over the words in the group and fill the corresponding cells
                 *  in the colors array with the group's color.
                 *  Cells not covered by any word keep the background color.
                 */
                for (uint8_t wI = 0; wI < arGroup.mWordsCount; wI++)
                {
                    tWord wWord = arGroup.mWords[wI];

                    if ((wWord > WORD_END_OF_WORDS) &&
                        (wWord < WORD_MAX_NUMBER))
                    {
                        const tWordData& wData = mcWordDataArray[wWord];

                        if ((wData.mRow < MATRIX_HEIGHT) &&
                            (wData.mColumn < MATRIX_WIDTH))
                        {
                            /* Calculate the end column for the word */
                            uint16_t wEndColumn = min(
                                static_cast<uint16_t>(wData.mColumn + wData.mLength),
                                static_cast<uint16_t>(MATRIX_WIDTH));

                            for (uint16_t wCol = wData.mColumn; wCol < wEndColumn; ++wCol)
                            {
                                wCellColors[wData.mRow][wCol] = wGroupColor;
                            }
                        }
                    }
                }
            }
        };

        /* Add word groups to the display preview */
        wAddGroup(apDisplayScene->mItIs);
        wAddGroup(apDisplayScene->mTime);
        wAddGroup(apDisplayScene->mExtraMinutes);
    }

    /**
     * Build SVG for display preview
     */

    /* Reserve 16 KB for the SVG string to avoid frequent reallocations */
    String wSvg;
    wSvg.reserve(16 * 1024);

    /* SVG element buffer */
    char wElement[128];

    /* Initialize SVG string */
    wSvg = "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 160 160' "
           "width='160' height='160' style='display:block;width:100%;height:auto;margin:0 auto'>";
    /* Add styles */
    wSvg += "<style type='text/css'>"
        "text{font-size:7px;font-weight:600;font-family:monospace;stroke-width:0.25;stroke:#404040}"
        ".none{fill:#000000}"
        "</style>";

    /* Add background rectangle in black color */
    wSvg += "<rect width='160' height='160' fill='#000000'/>";

    /* Add letters to the SVG */
    for (uint16_t wRow = 0; wRow < MATRIX_HEIGHT; ++wRow)
    {
        for (uint16_t wCol = 0; wCol < MATRIX_WIDTH; ++wCol)
        {
            /* Extract the letter from the display layout */
            char wLetter = mDisplayLayout[wRow][wCol * 3 + 2];

            if (apDisplayScene)
            {
                /* Format the SVG text element for the letter */
                snprintf(wElement, sizeof(wElement),
                        "<text x='%u' y='%u' fill='#%06lX'>%c</text>",
                        static_cast<unsigned>(wCol * 10 + 7),
                        static_cast<unsigned>(wRow * 10 + 7),
                        static_cast<unsigned long>(wCellColors[wRow][wCol]),
                        wLetter);
            }
            else
            {
                /* Format the SVG text element for the letter with the 'none' class */
                snprintf(wElement, sizeof(wElement),
                        "<text x='%u' y='%u' class='none'>%c</text>",
                        static_cast<unsigned>(wCol * 10 + 7),
                        static_cast<unsigned>(wRow * 10 + 7),
                        wLetter);
            }

            /* Append the formatted SVG element to the SVG string */
            wSvg += wElement;
        }
    }

    /* Close the SVG element */
    wSvg += "</svg>";

    LOG(LOG_DEBUG, "WebSite::BuildDisplayPreviewSvg() SVG length: %u", wSvg.length());

    return wSvg;
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

void WebSite::UpdateDisplayPreview(const MessageNS::Message& arMessage)
{
    DisplayScene wSceneSnapshot;
    mDisplaySceneSnapshot.Get(wSceneSnapshot);

    //ESPUI.updateLabel(mWebUIControlID.mDisplayPreview,
    //    BuildDisplayPreviewSvg(&wSceneSnapshot));
}

void WebSite::UpdateWiFiSettingsControls(bool aForceUpdate)
{
    tSSIDEntry wCurrentSsid;

    /* LOG */
    LOG(LOG_DEBUG, "WebSite::UpdateWiFiSettingsControls() Force update %d", aForceUpdate);

    /* Limit the SSID count to the UI's maximum supported options */
    size_t wCount = mSSIDList.Size();
    wCount = (wCount < mcMaxSsidCount) ? wCount : mcMaxSsidCount;

    const bool wHasSsids = (wCount > 0);

    /* Copy bounded SSID labels locally */
    for (size_t wI = 0; wI < wCount; ++wI)
    {
        tSSIDEntry wSsidEntry{};
        if (!mSSIDList.Get(wI, wSsidEntry))
        {
            wCount = wI;
            break;
        }

        auto& wLabel = mWifiSsidLabels[wI];
        snprintf(wLabel.data(), wLabel.size(), "%s", wSsidEntry.mSsid);

        if ((wI == 0) && !ESPUI.captivePortal)
        {
            wCurrentSsid.Set(wSsidEntry);
        }
    }

    /* Reuse controls and fixed-address labels instead of rebuilding the UI */
    for (size_t wI = 0; wI < wCount; wI++)
    {
        const char* wpLabel = mWifiSsidLabels[wI].data();

        if (wI < mWebUIControlID.mWifiSSIDList.size())
        {
            const auto wControlId = mWebUIControlID.mWifiSSIDList[wI];
            ESPUI.updateControlLabel(wControlId, wpLabel);
            ESPUI.updateControlValue(wControlId, static_cast<long>(wI));
        }
        else
        {
            const auto wControlId = ESPUI.addControl(Control::Type::Option, wpLabel,
                static_cast<long>(wI), Control::Color::None, mWebUIControlID.mWifiSSIDs);
            mWebUIControlID.mWifiSSIDList.push_back(wControlId);
        }
    }

    /* Remove surplus browser options, retaining their controls for later reuse */
    for (size_t wI = wCount; wI < mWebUIControlID.mWifiSSIDList.size(); ++wI)
    {
        ESPUI.updateControlValue(mWebUIControlID.mWifiSSIDList[wI], -2L);
    }

    /* Check if captive portal is enabled */
    if (ESPUI.captivePortal)
    {
        /* WiFi access point (offline) */
        wCurrentSsid.Clear();
    }
    else
    {
        /* Connected to WiFi router */

        if (!wHasSsids)
        {
            /* No SSIDs available, clear selected SSID */
            wCurrentSsid.Clear();

            LOG(LOG_ERROR, "WebSite::UpdateWiFiSettingsControls() Currently used SSID not available, clear selected SSID");
        }
    }

    ESPUI.updateSelect(mWebUIControlID.mWifiSSIDs, wHasSsids ? 0 : -1);
    ESPUI.updateText(mWebUIControlID.mWifiPassword, wCurrentSsid.mPassword);

    /* Restore scan button label */
    ESPUI.updateLabel(mWebUIControlID.mWifiScanButton, "Search for WiFi");

    /* Update forget network button label: "Forget network" or "Forget network <SSID>" if selected SSID is not valid */
    String wLabel = "Forget network";
    if (!ESPUI.captivePortal && wCurrentSsid.IsValid())
    {
        wLabel += " ";
        wLabel += wCurrentSsid.mSsid;
    }
    ESPUI.updateLabel(mWebUIControlID.mWifiForgetButton, wLabel);

    /* Enable/disable controls based on current state */
    ESPUI.setEnabled(mWebUIControlID.mWifiSSIDs,            true);
    ESPUI.setEnabled(mWebUIControlID.mWifiPassword,         true);
    ESPUI.setEnabled(mWebUIControlID.mWifiPasswordShowHide, true);
    ESPUI.setEnabled(mWebUIControlID.mWifiScanButton,       true);
    ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton,    false);
    ESPUI.setEnabled(mWebUIControlID.mWifiForgetButton,     (!ESPUI.captivePortal));   // Only enable forget button if connected to WiFi router

    if (aForceUpdate)
    {
        ESPUI.jsonReload();
    }
}

void WebSite::ControlCallback(BasicControl* apSender, int aType , void* apParam)
{
    if (apSender && apParam)
    {
        const ControlParam* wParam = static_cast<const ControlParam*>(apParam);

        switch (wParam->mHandler)
        {
            /* Handle WiFi controls */
            case ControlHandler::WifiSsidSelect:
            case ControlHandler::WifiPassword:
            case ControlHandler::WifiPasswordShowHide:
            case ControlHandler::WifiConnectButton:
            case ControlHandler::WifiScanButton:
            case ControlHandler::WifiForgetButton:
                if (mpWebSiteInstance)
                {
                    LOG(LOG_DEBUG, "WebSite::ControlCallback() WiFi control handler %d, type %d, sizeof type %d",
                        static_cast<int>(wParam->mHandler), aType, sizeof(aType));

                    MessageNS::Message wMessage{};
                    wMessage.mSource = MessageNS::tAddress::WEB_MANAGER;
                    wMessage.mDestination = MessageNS::tAddress::WEB_MANAGER;
                    wMessage.mId = MessageNS::tMessageId::CMD_WEBSITE_HANDLE_WIFI_CONTROLS;

                    /* Set payload 2 bytes:
                     *     1 byte  - ControlHandler number;
                     *     2..4 bytes - type (as int) */
                    const uint32_t wTypeRaw = static_cast<uint32_t>(static_cast<int32_t>(aType));

                    wMessage.mPayloadLength = 5;
                    wMessage.mPayload[0] = static_cast<uint8_t>(wParam->mHandler);
                    SerializeNS::SerializeData(wTypeRaw, wMessage.mPayload, 1);

                    mpWebSiteInstance->SendMessage(wMessage);
                }
                break;

            /* Handle other controls */
            default:
                if (mpWebSiteInstance)
                {
                    mpWebSiteInstance->HandleControl(apSender, aType, apParam);
                }
                break;
        }
    }
}
