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

/* CSS styles for the web UI controls */
#define CSS_STYLE_NONE                  ""
#define CSS_STYLE_GROUP                 "background-color: unset; width: 100%; text-align: center;"
#define CSS_STYLE_SEPARATOR             "background-color: unset; width: 100%;" // min-height:2px
#define CSS_STYLE_LABEL                 "background-color: unset; width: 100%; text-align: left;"
#define CSS_STYLE_INPUT                 "color: black; width: 100%; height:26px"
#define CSS_STYLE_SWITCH                "margin-bottom: 10px; align-items: center; text-align: right; vertical-align: middle;" //width: 12%; vertical-align: middle;"
#define CSS_STYLE_SWITCH_LABEL          "background-color: unset; width: 78%; text-align: left;"
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

    /* Initialize WEB UI but do not start it yet */

    /* ESPUI Log mode */
#if LOG_LEVEL == LOG_VERBOSE
    ESPUI.setVerbosity(Verbosity::Verbose);
#else
    /* Turn off verbose debugging */
    ESPUI.setVerbosity(Verbosity::Quiet);
#endif /* LOG_LEVEL == LOG_VERBOSE */

	/* Make sliders continually report their position as they are being dragged. */
	ESPUI.sliderContinuous = true;

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
    mWebUIControlID.mDisplayLedBrightness = AddPercentageSliderControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_INPUT,
            ConfigNS::mKeyDisplayLedBrightness, ConfigNS::mDefaultDisplayLedBrightness);

    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_SEPARATOR, "");

    /* Switcher for day/night mode activation */
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_SWITCH_LABEL, "Use day/night mode");

    mWebUIControlID.mDisplayUseNightMode = AddSwitcherControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_SWITCH,
            ConfigNS::mKeyDisplayUseNightMode, ConfigNS::mDefaultDisplayUseNightMode);
    /* Slider for night brightness selection */
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_LABEL, "Night mode brightness");
    mWebUIControlID.mDisplayBrightnessNightMode = AddPercentageSliderControl("", mWebUIControlID.mSettingsLedGroup, CSS_STYLE_INPUT,
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

            /* Start WEB UI */
            ESPUI.begin("Wordclock");
            break;

        case MessageNS::tMessageId::MSG_EVENT_WIFI_AP_STARTED:
            /* Access point started, start WEB server */
            LOG(LOG_DEBUG, "WebSite::ProcessIncomingMessage() Start web server in AP mode");

            /* Offline Mode - enable captive portal */
            ESPUI.captivePortal = true;

            /* Update WiFi settings controls */
            UpdateWiFiSettingsControls();

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

        case ControlHandler::WifiSsidSelect:
        {
            /* WiFi SSID selection changed */
            uint8_t wSsidIndex = ESPUI.getControl(mWebUIControlID.mWifiSSIDs)->getValueInt();

            if (wSsidIndex < mLocalSsidList.size() && (mLocalSsidList[wSsidIndex].mEncrypted))
            {
                String wPassw = ESPUI.getControl(mWebUIControlID.mWifiPassword)->getValue().c_str();

                // De-/activate button mWifiConnectButton
                ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton, (wPassw.length() > 0));
            }
        }
            break;

        case ControlHandler::WifiPassword:
        {
            /* WiFi password changed */
            String wPassw = ESPUI.getControl(mWebUIControlID.mWifiPassword)->getValue().c_str();

            /* De-/activate button mWifiConnectButton */
            ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton, (wPassw.length() > 0));
        }
            break;

        case ControlHandler::WifiPasswordShowHide:
        {
            /* WiFi password show/hide switcher changed */
            bool wState = (aType == S_ACTIVE) ? true : false;

            /* Set the input type of the password field based on the switcher state */
            ESPUI.setInputType(mWebUIControlID.mWifiPassword, wState ? "text" : "password");
        }
            break;

        case ControlHandler::WifiConnectButton:
        {
            /* Connect to selected network button pressed */
            if (aType == B_UP)
            {
                LOG(LOG_DEBUG, "WebSite::HandleControl() WiFi connect button pressed");

                // Get selected SSID and password from the controls
                uint8_t wSsidIndex = ESPUI.getControl(mWebUIControlID.mWifiSSIDs)->getValueInt();
                String wSsid  = mLocalSsidList[wSsidIndex].mSsid;
                String wPassw = ESPUI.getControl(mWebUIControlID.mWifiPassword)->getValue().c_str();

                LOG(LOG_DEBUG, "WebSite::HandleControl() Connecting to SSID: %s, password: %s", wSsid.c_str(), wPassw.c_str());

                Settings.SetValue<String>(ConfigNS::mKeyWifiSSID, wSsid.c_str());
                Settings.SetValue<String>(ConfigNS::mKeyWifiPassword, wPassw.c_str());

                /* Message to WiFi manager with connect command */
                wMessage.mSource = MessageNS::tAddress::WEB_MANAGER;
                wMessage.mDestination = MessageNS::tAddress::WIFI_MANAGER;
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

                ESPUI.setEnabled(mWebUIControlID.mWifiScanButton, false);
                ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton, false);

                ESPUI.jsonReload();

                /* Message to WiFi manager start scan command */
                wMessage.mSource = MessageNS::tAddress::WEB_MANAGER;
                wMessage.mDestination = MessageNS::tAddress::WIFI_MANAGER;
                wMessage.mId = MessageNS::tMessageId::CMD_WIFI_START_SCAN;

                wSendMessage = true;
            }
        }
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
    uint32_t wColorParam = 0;
    char wHexColor[10] = {0};

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wColorParam = Settings.GetValue<uint32_t>(aSettingsKey, aDefaultColor);
        sprintf(wHexColor, "#%06X", (wColorParam & 0x00FFFFFF));
    }

    Control::ControlId_t wControlId = AddTextControl(apLabel, aParent, aElementStyle, String(wHexColor), aSettingsKey, aHandler);
    ESPUI.setInputType(wControlId, "color");

    LOG(LOG_DEBUG, "WebSite::AddColorInput() Control %04X, param 0x%08X, color %s",
        wControlId, wColorParam, String(wHexColor).c_str());

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

        /* Initialize selected index with -1 (not found) */
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
            LOG(LOG_DEBUG, "WebSite::UpdateWiFiSettingsControls() Selected SSID not found, added and selected SSID: %s (Control ID: %04X)",
                    wSelectedSsid.c_str(), wControlId);
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

void WebSite::ControlCallback(BasicControl* apSender, int aType , void* apParam)
{
    if (mpWebSiteInstance)
    {
        mpWebSiteInstance->HandleControl(apSender, aType, apParam);
    }
}
