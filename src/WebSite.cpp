/*
 * WebSite.cpp
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#include <array>

#include <esp_ota_ops.h>

#include <ESPUI.h>

#include "Global.h"

#include "ProductInfo.h"
#include "DateTime.h"
#include "BitMatrix.h"
#include "DisplayLayout.h"
#include "OTAUpdate.h"

#include "WebSite.h"


/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)

/* CSS styles for the web UI controls */
#define CSS_STYLE_NONE                  ""
#define CSS_STYLE_GROUP                 "display: none;"
#define CSS_STYLE_SEPARATOR_GAP         "background-color: unset; width: 100%; height: 4px; min-height: 4px; box-sizing: border-box; line-height: 0; padding: 0; margin: 0;"
#define CSS_STYLE_SEPARATOR_LINE        "background-color: unset; width: 100%; height: 4px; min-height: 4px; box-sizing: border-box; padding: 0; margin: 0; border-top: 1px solid #808080;"
#define CSS_STYLE_LABEL_DEFAULT         "background-color: unset; text-align: left; padding: 0; margin-bottom: 4px; top: 0; vertical-align: bottom;"
#define CSS_STYLE_LABEL_H2              CSS_STYLE_LABEL_DEFAULT "width: 100%;"
#define CSS_STYLE_LABEL                 CSS_STYLE_LABEL_DEFAULT "font-weight: normal;  width: 100%;"
#define CSS_STYLE_LABEL_LIGHTER         CSS_STYLE_LABEL_DEFAULT "font-weight: 300; font-size: 0.9em; width: 100%;"
#define CSS_STYLE_LABEL_LIGHTER_HALF    CSS_STYLE_LABEL_DEFAULT "font-weight: 300; font-size: 0.9em; width: 50%;"
#define CSS_STYLE_INPUT_DEFAULT         "color: black; height: 26px; margin-left: 8px; box-sizing: border-box;"
#define CSS_STYLE_INPUT                 CSS_STYLE_INPUT_DEFAULT "width: calc(100% - 8px);"
#define CSS_STYLE_INPUT_HALF_WIDTH      CSS_STYLE_INPUT_DEFAULT "width: calc(50% - 8px);" // display: inline-block;
#define CSS_STYLE_SLIDER                "margin-left: 8px; width: calc(100% - 53px); box-sizing: border-box;"
#define CSS_STYLE_PREVIEW               "background-color: #000000; padding: 0; width: 100%; text-align: center;"
#define CSS_STYLE_SWITCH                "margin-bottom: 10px; align-items: center; text-align: right; vertical-align: middle;" // float: right; margin: 0 8px 10px 0;
#define CSS_STYLE_SWITCH_LABEL          "background-color: unset; font-weight: normal; width: calc(100% - 68px); text-align: left; padding: 0; vertical-align: middle;"
#define CSS_STYLE_BUTTON                "width: 100%" //; font-size:1.6rem;min-height:4rem;padding:1rem;margin:3px;border:2px solid #404040;border-radius:5px;"
#define CSS_STYLE_TABEL                 "width:100%;border-collapse:collapse;text-align:left;white-space:normal;"
#define CSS_STYLE_TABEL_NAME_CELL       "width:38%;padding:5px 8px 5px 0;vertical-align:top;"
#define CSS_STYLE_TABEL_VALUE_CELL      "padding:5px 0;overflow-wrap:anywhere;word-break:break-word;vertical-align:top;"
#define CSS_STYLE_TABEL_ROW_BORDER      "border-bottom:1px solid #808080;"

/** Periodical task timer period in milliseconds */
#define PERIODICAL_TIMER_PERIOD         15000   // 15 seconds

/* Delay for web UI updates in milliseconds */
#define WEB_UI_UPDATE_DELAY             100     // It seems that a small delay is enough to avoid issues with the web UI updates, especially when updating control states

/* Default control colors */
#define TAB_CONTROL_COLOR               Control::Color::Wetasphalt
#define GROUP_CONTROL_COLOR             Control::Color::Wetasphalt

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
    /* Clean up task timers */
    if (mpTimer)
    {
        /* Stop running timer */
        mpTimer->stop();
        /*    and destroy it */
        delete mpTimer;
        mpTimer = nullptr;
    }

    /* Delete the control event queue and notification */
    delete mpControlEventQueue;
    delete mpControlEventNotification;

    mpControlEventQueue = nullptr;
    mpControlEventNotification = nullptr;

    /* Clear "this" static pointer */
    mpWebSiteInstance = nullptr;
}

void WebSite::Init(ApplicationNS::tTaskObjects* apTaskObjects)
{
    /* Initialize base class */
    ApplicationNS::Task::Init(apTaskObjects);

    /* Create a queue for ESPUI control events */
    mpControlEventQueue = new ControlEventQueue();
    if (mpControlEventQueue == nullptr)
    {
        LOG(LOG_ERROR, "WebSite::Init() Failed to create control event queue");
    }

    /* Create a task notification for control events */
    mpControlEventNotification = new TaskNotification(getTaskHandle(), mTaskNotificationControlEvent);
    if (mpControlEventNotification == nullptr)
    {
        LOG(LOG_ERROR, "WebSite::Init() Failed to create control event notification");
    }

    /* Create periodical timer */
    mTimerObjects.mTaskHandle = this->getTaskHandle();
    mTimerObjects.mpTaskMessagesQueue = this->mpTaskObjects->mpMessageQueue;
    mpTimer = new ApplicationNS::TaskTimer(mPeriodicalTaskTimerId, PERIODICAL_TIMER_PERIOD, true);
    mpTimer->Init(&mTimerObjects);

    /* Initialize WEB UI but do not start it yet */
    InitWebUIControls();
}

void WebSite::OnOtaUploadStatusChanged(OTAUpdateNS::OtaUploadStatus aStatus, esp_err_t aError, const String& arFailureReason)
{
    switch (aStatus)
    {
        case OTAUpdateNS::OtaUploadStatus::Receiving:
        case OTAUpdateNS::OtaUploadStatus::ReadyToBoot:
            /* OTA upload process is ongoing */

            mIsWebUIUpdateEnabled = false;  // Disable web UI updates during OTA upload
            break;

        case OTAUpdateNS::OtaUploadStatus::Idle:
        case OTAUpdateNS::OtaUploadStatus::Failed:
        default:
            /* OTA upload process is not active */

            mIsWebUIUpdateEnabled = true;  // Re-enable web UI updates after OTA upload
            break;
    }
}

void WebSite::ProcessTimerEvent(const uint32_t aTimerId)
{
    LOG(LOG_VERBOSE, "WebSite::ProcessTimerEvent() Timer ID: %d", aTimerId);

    if (aTimerId == mPeriodicalTaskTimerId)
    {
        /* Update the system information only if web UI updates are enabled */
        if (mIsWebUIUpdateEnabled)
        {
            UpdateSystemInfo();
        }
    }
    else
    {
        // do nothing here
    }
}

void WebSite::ProcessIncomingMessage(const MessageNS::Message &arMessage)
{
    LOG(LOG_VERBOSE, "WebSite::ProcessIncomingMessage()");

    switch (arMessage.mId)
    {
        case MessageNS::tMessageId::MSG_EVENT_WIFI_STA_CONNECTED:
            /* WiFi connected in STA mode, start WEB server */
            StartWebServer(false);  // no captive portal
            break;

        case MessageNS::tMessageId::MSG_EVENT_WIFI_AP_STARTED:
            /* WiFi connected in AP mode, start WEB server */
            StartWebServer(true);   // enable captive portal
            break;

        case MessageNS::tMessageId::MSG_EVENT_WIFI_SCAN_DONE:
            /* WiFi scan finished, update WiFi settings */
            UpdateWiFiSettingsControls();
            break;

        case MessageNS::tMessageId::MSG_EVENT_DISPLAY_FRAME_READY:
            /* Update display preview only if web UI updates are enabled */
            if (mIsWebUIUpdateEnabled)
            {
                UpdateDisplayPreview(arMessage);
            }
            break;

        default:
            // do nothing
            break;
    }
}

void WebSite::ProcessUnknownNotification(uint32_t aNotificationValue)
{
    /* Process control event notification */
    if ((aNotificationValue & mTaskNotificationControlEvent) != 0)
    {
        ControlEvent wEvent{};

        /* Process all pending control events */
        while (mpControlEventQueue->pop(wEvent, 0))
        {
            switch (wEvent.mControlParam.mHandler)
            {
                /* Handle WiFi controls */
                case ControlHandler::WifiSsidSelect:
                case ControlHandler::WifiPassword:
                case ControlHandler::WifiPasswordShowHide:
                case ControlHandler::WifiConnectButton:
                case ControlHandler::WifiScanButton:
                case ControlHandler::WifiForgetButton:
                    HandleWiFiControlEvent(wEvent);
                    break;

                /* Handle other controls */
                default:
                    HandleControlEvent(wEvent);
                    break;
            }

            /* Allow other tasks to run */
            yield();
        }

        /* Clear the control event notification bit */
        aNotificationValue &= ~mTaskNotificationControlEvent;
    }
}

/** @brief Handle a control event
 *
 * @param arEvent The control event to handle
 */
void WebSite::HandleControlEvent(const ControlEvent& arEvent)
{
    /* Flag to indicate whether to send a message or not */
    bool wSendMessage = false;  // default - false

    SettingsNS::tKey wSettingsKey = arEvent.mControlParam.mSettingsKey;

    if (wSettingsKey != ConfigNS::mInvalidKey)
    {
        /* Handle control based on its handler tag */
        switch (arEvent.mControlParam.mHandler)
        {
            case ControlHandler::Text:
            {
                /* LOG */
                LOG(LOG_DEBUG, "WebSite::HandleControlEvent() Text control %04X, new value %s",
                        arEvent.mControlId, arEvent.mPayload.mParameter.mValueText);

                /* Store new text value in settings */
                wSendMessage = Settings.SetValue<String>(wSettingsKey, arEvent.mPayload.mParameter.mValueText);
            }
                break;

            case ControlHandler::Color:
            {
                /* Retrieve the color value */
                std::string wColorStr = arEvent.mPayload.mParameter.mValueText;
                uint32_t  wColorValue = std::stoi(wColorStr.substr(1), nullptr, 16); // skip '#'

                /* Update displayed value */
                ESPUI.updateText(arEvent.mControlId, arEvent.mPayload.mParameter.mValueText);

                /* LOG */
                LOG(LOG_DEBUG, "WebSite::HandleControlEvent() Color control %04X, new color %s (0x%08X)",
                        arEvent.mControlId, wColorStr.c_str(), wColorValue);

                /* Store new color value as uint32_t in settings */
                wSendMessage = Settings.SetValue<uint32_t>(wSettingsKey, wColorValue);
            }
                break;

            case ControlHandler::Timer:
            {
                /* Retreive new time value in format HH:MM */
                std::string wTimeStr = arEvent.mPayload.mParameter.mValueText;

                uint8_t wHour   = static_cast<uint8_t>(std::stoi(wTimeStr.substr(0, 2)));
                uint8_t wMinute = static_cast<uint8_t>(std::stoi(wTimeStr.substr(3, 2)));

                /* LOG */
                LOG(LOG_DEBUG, "WebSite::HandleControlEvent() Timer control %04X, new time %02u:%02u",
                        arEvent.mControlId, wHour, wMinute);

                /* Convert to uint32_t in format HHMM */
                DateTimeNS::tDateTime wDateTime;
                wDateTime.mTime.mHour   = wHour;
                wDateTime.mTime.mMinute = wMinute;
                wDateTime.mTime.mSecond = 0;
                wDateTime.mDate.mDay    = 1;
                wDateTime.mDate.mMonth  = 1;
                wDateTime.mDate.mYear   = DateTimeNS::mYearRangeStart;

                uint32_t wTimeDword = DateTimeNS::DateTimeToDword(wDateTime);

                /* Store new time value as uint32_t in settings */
                wSendMessage = Settings.SetValue<uint32_t>(wSettingsKey, wTimeDword);
            }
                break;

            case ControlHandler::Switcher:
            {
                /* Retrieve new switcher state */
                bool wState = (arEvent.mPayload.mParameter.mValue != 0);

                LOG(LOG_DEBUG, "WebSite::HandleControlEvent() Switcher control %04X, new state %d",
                        arEvent.mControlId, wState);

                /* Store new text value as bool in settings*/
                wSendMessage = Settings.SetValue<bool>(wSettingsKey, wState);
            }
                break;

            case ControlHandler::Select:
            {
                /* Retrieve new selected option */
                uint8_t wSelectedOption = static_cast<uint8_t>(arEvent.mPayload.mParameter.mValue);

                /* LOG */
                LOG(LOG_DEBUG, "WebSite::HandleControlEvent() Select control %04X, type %d, selection %d",
                        arEvent.mControlId, arEvent.mEventType, wSelectedOption);

                /* Store new selected option as uint8_t in settings */
                wSendMessage = Settings.SetValue<uint8_t>(wSettingsKey, wSelectedOption);
            }
                break;

            case ControlHandler::Slider:
            {
                /* Retrieve new slider value */
                uint8_t wValue = static_cast<uint8_t>(arEvent.mPayload.mParameter.mValue);

                /* LOG */
                LOG(LOG_DEBUG, "WebSite::HandleControlEvent() Slider control %04X, new value %d",
                        arEvent.mControlId, wValue);

                /* Store new slider value as uint8_t in settings */
                wSendMessage = Settings.SetValue<uint8_t>(wSettingsKey, wValue);
            }
                break;

            default:
                /* LOG */
                LOG(LOG_ERROR, "WebSite::HandleControl() Unknown control handler for control ID %04X", arEvent.mControlId);
                break;
        }
    }

    /* Send message if needed */
    if (wSendMessage)
    {
        /* Prepare message */
        MessageNS::Message wMessage;

        wMessage.mSource = MessageNS::tAddress::WEB_MANAGER;
        wMessage.mId = MessageNS::tMessageId::MSG_EVENT_SETTINGS_CHANGED;

        /* Send message to display */
        wMessage.mDestination = MessageNS::tAddress::DISPLAY_MANAGER;
        SendMessage(wMessage);

        /* Send message to time manager */
        wMessage.mDestination = MessageNS::tAddress::TIME_MANAGER;
        SendMessage(wMessage);
    }
}

/** @brief Handle a WiFi control event
 *
 * @param arEvent The WiFi control event to handle
 */
void WebSite::HandleWiFiControlEvent(const ControlEvent& arEvent)
{
    /* Message to be sent if control changes */
    MessageNS::Message wMessage;
    /* Flag to indicate whether to send a message or not */
    bool wSendMessage = false;  // default - false

    /* Prepare message */
    wMessage.mSource = MessageNS::tAddress::WEB_MANAGER;
    wMessage.mDestination = MessageNS::tAddress::WIFI_MANAGER;

    /* Handle control based on its handler tag */
    switch (arEvent.mControlParam.mHandler)
    {
        case ControlHandler::WifiSsidSelect:
        case ControlHandler::WifiPassword:
        {
            bool wIsSsidValid     = false;
            bool wIsPasswordValid = false;
            tSSIDEntry wSelectedSsid{};

            /* Get current WiFi SSID selection */
            const long wSsidIndex = arEvent.mPayload.mWifi.mSsidIndex;
            /* Use the password snapshot captured with this event. */
            String wPassw = arEvent.mPayload.mWifi.mPassword;

            wIsSsidValid = (wSsidIndex >= 0) &&
                (static_cast<size_t>(wSsidIndex) < mcMaxSsidCount) &&
                mSSIDList.Get(static_cast<size_t>(wSsidIndex), wSelectedSsid) &&    // Copy the selected entry
                wSelectedSsid.IsValid();

            if (wIsSsidValid)
            {
                /**
                 * Allows any valid SSID if Wifi AP mode is active,
                 * otherwise, excludes index 0 (the current network)
                 */
                wIsSsidValid = mIsWifiAPMode || (wSsidIndex > 0);

                /* Check password validity: it is non-empty and the selected network is encrypted */
                wIsPasswordValid = ((wPassw.length() > 7) && wSelectedSsid.mEncrypted);    // Technical minimum length: 8 characters
            }

            ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton, (wIsSsidValid && wIsPasswordValid));

        }
            break;

        case ControlHandler::WifiPasswordShowHide:
        {
            /* WiFi password show/hide switcher changed */
            bool wState = (arEvent.mEventType == S_ACTIVE);

            /* Set the input type of the password field based on the switcher state */
            ESPUI.setInputType(mWebUIControlID.mWifiPassword, wState ? "text" : "password");

            /* Wait a bit to avoid issues with the UI update */
            delay(WEB_UI_UPDATE_DELAY);
        }
            break;

        case ControlHandler::WifiConnectButton:
        {
            /* Connect to selected network button pressed */
            if (arEvent.mEventType == B_UP)
            {
                LOG(LOG_DEBUG, "WebSite::HandleControl() WiFi connect button pressed");

                /* Get selected SSID and password from the controls */
                const long wSsidIndex = arEvent.mPayload.mWifi.mSsidIndex;
                tSSIDEntry wSelectedSsid{};
                const bool wIsSsidValid = (wSsidIndex >= 0) &&
                    (static_cast<size_t>(wSsidIndex) < mcMaxSsidCount) &&
                    mSSIDList.Get(static_cast<size_t>(wSsidIndex), wSelectedSsid) &&
                    wSelectedSsid.IsValid();
                if (!wIsSsidValid)
                {
                    LOG(LOG_ERROR, "WebSite::HandleWiFiControls() Ignoring connect request with invalid SSID index %ld", wSsidIndex);
                    break;
                }

                String wSsid = wSelectedSsid.mSsid;
                String wPassw = arEvent.mPayload.mWifi.mPassword;

                LOG(LOG_DEBUG, "WebSite::HandleWiFiControls() Connecting to SSID: %s", wSsid.c_str());

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
            if (arEvent.mEventType == B_UP)
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

                /* Wait a bit to avoid issues with the UI update */
                delay(WEB_UI_UPDATE_DELAY);

                /* Message to WiFi manager start scan command */
                wMessage.mId = MessageNS::tMessageId::CMD_WIFI_START_SCAN;
                wSendMessage = true;
            }
        }
            break;

        case ControlHandler::WifiForgetButton:
        {
            /* Forget WiFi network button pressed */
            if (arEvent.mEventType == B_UP)
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
            LOG(LOG_ERROR, "WebSite::HandleWiFiControls() Unknown control handler %d",
                static_cast<int>(arEvent.mControlParam.mHandler));
            break;
    }

    /* Send message if needed */
    if (wSendMessage)
    {
        SendMessage(wMessage);
    }
}

/**
 * @brief Initialize the web UI controls
 *
 * @details
 * This function sets up the initial state of the Web UI controls.
 */
void WebSite::InitWebUIControls(void)
{
    /* ESPUI log mode - turn on/off verbose debugging */
    ESPUI.setVerbosity((LOG_LEVEL == LOG_VERBOSE) ? Verbosity::Verbose : Verbosity::Quiet);

    /* Send slider changes only when the user releases the slider. */
    ESPUI.sliderContinuous = false;     // Disable continuous slider mode


    /* Add the tabs for the web UI */
    mWebUIControlID.mOverViewTab   = ESPUI.addControl(Control::Type::Tab, "overview",    "Overview",    TAB_CONTROL_COLOR);
    mWebUIControlID.mLedDisplayTab = ESPUI.addControl(Control::Type::Tab, "led-display", "LED Display", TAB_CONTROL_COLOR);
    mWebUIControlID.mSettingsTab   = ESPUI.addControl(Control::Type::Tab, "settings",    "Settings",    TAB_CONTROL_COLOR);


    /**
     * Tab Overview / Group Preview
     */
    mWebUIControlID.mPreviewGroup = AddGroupHelper("Preview", mWebUIControlID.mOverViewTab, GROUP_CONTROL_COLOR);

    /* Live preview of the word-clock LED matrix */
    mWebUIControlID.mDisplayPreview = AddLabelControl("", mWebUIControlID.mPreviewGroup, CSS_STYLE_PREVIEW,
        BuildDisplayPreviewSvg()); // "Here will be a live preview of the word-clock LED matrix. Please wait until the first update is received from the device


    /**
     * Tab LED Display / Group Display
     */
    mWebUIControlID.mLedDisplayDisplayGroup = AddGroupHelper("Display", mWebUIControlID.mLedDisplayTab, GROUP_CONTROL_COLOR);

    /* Clock mode */
    AddLabelControl("", mWebUIControlID.mLedDisplayDisplayGroup, CSS_STYLE_LABEL, "Clock mode");
    mWebUIControlID.mDisplayClockMode = AddSelectControl("", mWebUIControlID.mLedDisplayDisplayGroup, CSS_STYLE_INPUT,
            mcClockModeItems, WORDCLOCK_MODE_NUMBER,
            ConfigNS::mKeyDisplayClockMode, mcDefaultDisplayClockMode);

    /* Switcher for 'IT IS' words */
    AddLabelControl("", mWebUIControlID.mLedDisplayDisplayGroup, CSS_STYLE_SWITCH_LABEL, "Show 'IT IS'");
    mWebUIControlID.mDisplayClockItIs = AddSwitcherControl("", mWebUIControlID.mLedDisplayDisplayGroup, CSS_STYLE_SWITCH,
            ConfigNS::mKeyDisplayClockItIs, ConfigNS::mDefaultDisplayClockItIs);

    /* Switch for single minutes */
    AddLabelControl("", mWebUIControlID.mLedDisplayDisplayGroup, CSS_STYLE_SWITCH_LABEL, "Show single minutes");
    mWebUIControlID.mDisplayClockSingleMinutes = AddSwitcherControl("", mWebUIControlID.mLedDisplayDisplayGroup, CSS_STYLE_SWITCH,
            ConfigNS::mKeyDisplayClockSingleMins, ConfigNS::mDefaultDisplayClockSingleMins);


    /**
     * Tab LED Display / Group Color
     */
    mWebUIControlID.mLedDisplayColorGroup = AddGroupHelper("Colors", mWebUIControlID.mLedDisplayTab, GROUP_CONTROL_COLOR);

    /* Day time and background colors */
    AddLabelControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_LABEL, "Day");
    AddLabelControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_LABEL_LIGHTER_HALF, "time");
    AddLabelControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_LABEL_LIGHTER_HALF, "background");

    mWebUIControlID.mDisplayColorTime = AddColorControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_INPUT_HALF_WIDTH,
        ConfigNS::mKeyDisplayColorTime, ConfigNS::mDefaultDisplayColorTime);
    mWebUIControlID.mDisplayColorBkgd = AddColorControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_INPUT_HALF_WIDTH,
        ConfigNS::mKeyDisplayColorBkgd, ConfigNS::mDefaultDisplayColorBkgd);

    /* Separator */
    AddSeparator(mWebUIControlID.mLedDisplayColorGroup);

    /* Night time and background colors */
    AddLabelControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_LABEL, "Night");
    AddLabelControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_LABEL_LIGHTER_HALF, "time");
    AddLabelControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_LABEL_LIGHTER_HALF, "background");

    mWebUIControlID.mDisplayColorTimeNight = AddColorControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_INPUT_HALF_WIDTH,
        ConfigNS::mKeyDisplayColorTimeNight, ConfigNS::mDefaultDisplayColorTimeNight);
    mWebUIControlID.mDisplayColorBkgdNight = AddColorControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_INPUT_HALF_WIDTH,
        ConfigNS::mKeyDisplayColorBkgdNight, ConfigNS::mDefaultDisplayColorBkgdNight);


    /**
     * Tab LED Display / Group Brightness
     */
    mWebUIControlID.mLedDisplayBrightnessGroup = AddGroupHelper("Brightness", mWebUIControlID.mLedDisplayTab, GROUP_CONTROL_COLOR);

    /* Switcher for day/night mode activation */
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_SWITCH_LABEL, "Use day/night mode");
    mWebUIControlID.mDisplayUseDayNightMode = AddSwitcherControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_SWITCH,
            ConfigNS::mKeyDisplayUseDayNightMode, ConfigNS::mDefaultDisplayUseDayNightMode);

    /* Time input for day mode start/end */
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL, "Day mode");
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL_LIGHTER_HALF, "start time");
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL_LIGHTER_HALF, "end time");
    mWebUIControlID.mDisplayDayModeStartTime = AddTimeControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_INPUT_HALF_WIDTH,
            ConfigNS::mKeyDisplayDayModeStartTime, ConfigNS::mDefaultDisplayDayModeStartTime);
    mWebUIControlID.mDisplayDayModeEndTime = AddTimeControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_INPUT_HALF_WIDTH,
            ConfigNS::mKeyDisplayDayModeEndTime, ConfigNS::mDefaultDisplayDayModeEndTime);

    AddSeparator(mWebUIControlID.mLedDisplayBrightnessGroup, true); // Draw a line separator

    /* Sliders for day time/background brightness */
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL, "Day");
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL_LIGHTER, "time");
    mWebUIControlID.mDisplayBrightnessTime = AddPercentageSliderControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_SLIDER,
        ConfigNS::mKeyDisplayBrightnessTime, ConfigNS::mDefaultDisplayBrightnessTime);
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL_LIGHTER, "background");
    mWebUIControlID.mDisplayBrightnessBkgd = AddPercentageSliderControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_SLIDER,
            ConfigNS::mKeyDisplayBrightnessBkgd, ConfigNS::mDefaultDisplayBrightnessBkgd);

    AddSeparator(mWebUIControlID.mLedDisplayBrightnessGroup);

    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL, "Night");
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL_LIGHTER, "time");
    mWebUIControlID.mDisplayBrightnessTimeNight = AddPercentageSliderControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_SLIDER,
        ConfigNS::mKeyDisplayBrightnessTimeNight, ConfigNS::mDefaultDisplayBrightnessTimeNight);
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL_LIGHTER, "background");
    mWebUIControlID.mDisplayBrightnessBkgdNight = AddPercentageSliderControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_SLIDER,
        ConfigNS::mKeyDisplayBrightnessBkgdNight, ConfigNS::mDefaultDisplayBrightnessBkgdNight);


    /**
     * Tab Settings / Group Time
     */
    mWebUIControlID.mSettingsTimeGroup = AddGroupHelper("Time", mWebUIControlID.mSettingsTab, GROUP_CONTROL_COLOR);

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
     * Tab Settings / Group WiFi
     */
    mWebUIControlID.mSettingsWiFiGroup = AddGroupHelper("WiFi Access Point Credentials", mWebUIControlID.mSettingsTab, GROUP_CONTROL_COLOR);

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


    /**
     * Tab Settings / Group System Information
     */
    mWebUIControlID.mSystemInfoGroup = AddGroupHelper("System Informations", mWebUIControlID.mSettingsTab, GROUP_CONTROL_COLOR);

    mWebUIControlID.mProductInfoLabel = AddLabelControl("", mWebUIControlID.mSystemInfoGroup, CSS_STYLE_LABEL "display:block;white-space:normal",
        "");    // Placeholder for product information, will be updated in UpdateProductInfo()
    AddSeparator(mWebUIControlID.mSystemInfoGroup, true);
    mWebUIControlID.mRuntimeInfoLabel = AddLabelControl("", mWebUIControlID.mSystemInfoGroup, CSS_STYLE_LABEL "display:block;white-space:normal",
        "");    // Placeholder for runtime information, will be updated in UpdateRuntimeInfo()
    AddSeparator(mWebUIControlID.mSystemInfoGroup, true);
    mWebUIControlID.mDeviceInfoLabel = AddLabelControl("", mWebUIControlID.mSystemInfoGroup, CSS_STYLE_LABEL "display:block;white-space:normal",
        "");    // Placeholder for device information, will be updated in UpdateDeviceInfo()

    /**
     * Tab Settings / Firmware Update Group
     */
    mWebUIControlID.mFirmwareUpdateGroup = AddGroupHelper("Firmware Update", mWebUIControlID.mSettingsTab, GROUP_CONTROL_COLOR);

    /* Update the system information displayed on the web UI */
    UpdateSystemInfo();
}

/**
 * @brief Start the web server
 *
 * @details
 * This function initializes and starts the web server, optionally enabling
 * WiFi Access Point (AP) mode with a captive portal. It also updates the WiFi
 * settings controls and registers the necessary routes for OTA updates.
 *
 * @param aIsWifiAPMode True to start in WiFi AP mode with captive portal; false for normal mode
 */
void WebSite::StartWebServer(bool aIsWifiAPMode)
{
    LOG(LOG_DEBUG, "WebSite::StartWebServer() Starting web server, Captive Portal: %d",
            aIsWifiAPMode);

    /* Set WiFi AP mode flag */
    mIsWifiAPMode = aIsWifiAPMode;

    /* Set captive portal mode for ESPUI */
    ESPUI.captivePortal = mIsWifiAPMode;

    /* Update WiFi settings controls */
    UpdateWiFiSettingsControls();

    /* Set custom JavaScript for OTA update handling */
    ESPUI.setCustomJS(OTAUpdateNS::CustomJavaScript("de"));

    /* Start WEB UI */
    ESPUI.begin("Wordclock");

    /* Register OTA update routes */
    OTAUpdateNS::RegisterRoutes(ESPUI.WebServer(), this);

    /* Start notification timer for this task */
    mpTimer->start();
}

/**
 * @brief Helper function to add a group control to the web UI
 * @param apLabel The label for the group control
 * @param aParent The parent control ID (default: no parent)
 * @param aColor The color of the group control (default: None)
 * @param aExpandable Whether the group control is expandable (default: false)
 * @param aExpanded Whether an expandable group starts open (default: false)
 *
 * @return The control ID of the newly added group control
 */
Control::ControlId_t WebSite::AddGroupHelper(const char * apLabel, Control::ControlId_t aParent, Control::Color aColor,
    const bool aExpandable, const bool aExpanded)
{
	Control::ControlId_t wControlId = Control::noParent;

    if (aExpandable)
    {
        wControlId = ESPUI.addControl(Control::Type::Details, apLabel, "", aColor, aParent);

        ESPUI.updateDetails(wControlId, aExpanded);
    }
    else
    {
        wControlId = ESPUI.addControl(Control::Type::Label, apLabel, "", aColor, aParent);
    }

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
 * @brief Add a visual separator or a small gap to the web UI
 * @param aParent The parent control ID
 * @param aDrawLine True to draw a horizontal line; false to add a small gap
 * @return The control ID of the separator label
 */
Control::ControlId_t WebSite::AddSeparator(Control::ControlId_t aParent, bool aDrawLine)
{
    const char* wpStyle = aDrawLine ? CSS_STYLE_SEPARATOR_LINE : CSS_STYLE_SEPARATOR_GAP;
    return AddLabelControl("", aParent, wpStyle, "");
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

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::ColorPickr, apLabel, String(wHexColor), Control::Color::Dark, aParent,
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
        wBackgroundColor = apDisplayScene->mColorBkgd & COLOR_RGB_MASK;

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

/**
 * Builds an HTML table from the given names and values.
 *
 * @param arNames  An array of names for the table rows.
 * @param arValues An array of values corresponding to the names.
 * @param aSeparateRows If true, selparate rows will be created for each name-value pair;
 *                      if false, a single row will be created with all names and values.
 * @return A string containing the HTML representation of the table.
 */
template <size_t N>
String WebSite::BuildHtmlTable(const std::array<const char*, N>& arNames,
    const std::array<const char*, N>& arValues, bool aSeparateRows) const
{
    /* Initialize the HTML table string */
    String wHtml = "<table style='" CSS_STYLE_TABEL "'>";

    /* Build the table rows */
    for (size_t wI = 0; wI < N; wI++)
    {
        /* Determine if a bottom border should be applied to the row */
        const char* wpBottomBorder = "";
        if (aSeparateRows && (wI + 1 < N))
        {
            wpBottomBorder = CSS_STYLE_TABEL_ROW_BORDER;
        }

        /* Build the HTML for the current row */
        wHtml += "<tr><td style='" CSS_STYLE_TABEL_NAME_CELL;
        wHtml += wpBottomBorder;
        wHtml += "'>";
        wHtml += EscapeHtml(arNames[wI]);
        wHtml += "</td><td style='" CSS_STYLE_TABEL_VALUE_CELL;
        wHtml += wpBottomBorder;
        wHtml += "'>";
        wHtml += EscapeHtml(arValues[wI]);
        wHtml += "</td></tr>";
    }

    /* Close the HTML table */
    wHtml += "</table>";

    return wHtml;
}

/**
 * Escapes special HTML characters in the given text.
 *
 * @param apText The text to escape.
 * @return A string with special HTML characters replaced by their corresponding HTML entities.
 */
String WebSite::EscapeHtml(const char* apText) const
{
    String wEscaped = apText;

    /* Replace special characters with their HTML entity equivalents */
    wEscaped.replace("&",  "&amp;");
    wEscaped.replace("<",  "&lt;");
    wEscaped.replace(">",  "&gt;");
    wEscaped.replace("\"", "&quot;");
    wEscaped.replace("'",  "&#39;");

    return wEscaped;
}

void WebSite::UpdateDisplayPreview(const MessageNS::Message& arMessage)
{
    DisplayScene wSceneSnapshot;
    mDisplaySceneSnapshot.Get(wSceneSnapshot);

    ESPUI.updateLabel(mWebUIControlID.mDisplayPreview,
        BuildDisplayPreviewSvg(&wSceneSnapshot));
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

        if ((wI == 0) && !mIsWifiAPMode)
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

    /* Check if Wifi AP mode is enabled */
    if (mIsWifiAPMode)
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
    if (!mIsWifiAPMode && wCurrentSsid.IsValid())
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
    ESPUI.setEnabled(mWebUIControlID.mWifiForgetButton,     (!mIsWifiAPMode));   // Only enable forget button if connected to WiFi router

    if (aForceUpdate)
    {
        ESPUI.jsonReload();
    }
}

/**
 * @brief Update the product information displayed on the web UI.
 *
 * @details
 * This function retrieves product information such as software version, build date/time,
 * branch, and commit hash from the configuration and updates the corresponding controls
 * on the web UI. If any of the information is unavailable, default values are used.
 */
void WebSite::UpdateSystemInfo(void)
{
    /* Update product information */
    {
        static bool wProductInfoInitialized = false;

        if (!wProductInfoInitialized)
        {
            /* Initialize product information on the web UI */
            wProductInfoInitialized = true;

            String wSoftwareVersion = "Unknown";
            String wBuildDateTime   = "Unavailable";

            /* Retrieve product information record */
            ProductInfoNS::Record wProductInfo = ProductInfoNS::GetProductInfoRecord();

            if (wProductInfo.mSoftwareVersion.IsValid())
            {
                char wVersionText[12] = {0};
                snprintf(wVersionText, sizeof(wVersionText), "%u.%u.%u",
                    static_cast<unsigned>(wProductInfo.mSoftwareVersion.mMajor),
                    static_cast<unsigned>(wProductInfo.mSoftwareVersion.mMinor),
                    static_cast<unsigned>(wProductInfo.mSoftwareVersion.mBuild));

                wSoftwareVersion = wVersionText;
            }

            if (wProductInfo.mBuildDateTimeLocal[0] != '\0')
            {
                wBuildDateTime = wProductInfo.mBuildDateTimeLocal;
            }

            const std::array<const char*, 2> wProductInfoNames = {
                "Software version", "Build date/time"
            };

            /* Update the product information label on the web UI with the constructed HTML table */
            ESPUI.updateLabel(mWebUIControlID.mProductInfoLabel,
                BuildHtmlTable(wProductInfoNames, {wSoftwareVersion.c_str(), wBuildDateTime.c_str()}));
        }
    }

    /* Runtime information */
    {
        /* Static variables to track initialization and previous uptime */
        static bool wRuntimeInitialized = false;
        static DateTimeNS::tUptime wPreviousUptime{};

        DateTimeNS::tUptime wUptime = DateTimeNS::GetUptime();

        if ((!wRuntimeInitialized) ||
            ((wUptime.ToWord()         & MASK_UPTIME_WITHOUT_SECONDS) !=    // Ignore the six least-significant second bits
             (wPreviousUptime.ToWord() & MASK_UPTIME_WITHOUT_SECONDS)))     //  when comparing uptime words
        {
            wRuntimeInitialized = true;

            /* Update previous uptime for future comparisons */
            wPreviousUptime = wUptime;

            char wUptimeText[15]{};     // Longest output: '32767d 23h 59m' (14 characters plus NUL)
            snprintf(wUptimeText, sizeof(wUptimeText), "%ud %02uh %02um",
                static_cast<unsigned>(wUptime.wDays),
                static_cast<unsigned>(wUptime.wHours),
                static_cast<unsigned>(wUptime.wMinutes));

            String wRestartReason = RestartReasonNS::ToString(
                                            RestartReasonNS::GetRestartReason());

            const std::array<const char*, 2> wRuntimeInfoNames = {
                "Runtime", "Restart reason"
            };

            /* Update the runtime information label on the web UI. */
            ESPUI.updateLabel(mWebUIControlID.mRuntimeInfoLabel,
                BuildHtmlTable(wRuntimeInfoNames, {wUptimeText, wRestartReason.c_str()}));
        }
    }

    /* Device information */
    {
        /* Static variables to track initialization and previous device metrics */
        static bool wDeviceInfoInitialized = false;
        static std::array<uint32_t, 6> wPreviousDeviceMetrics{};

        /* Retrieve device metrics */
        const esp_partition_t* wpRunningPartition = esp_ota_get_running_partition();
        const uint32_t wPartitionSize =
            (wpRunningPartition != nullptr) ? wpRunningPartition->size : 0U;
        const uint32_t wSketchSize = ESP.getSketchSize();
        const uint32_t wHeapSize = ESP.getHeapSize();
        const uint32_t wFreeHeap = ESP.getFreeHeap();
        const uint32_t wMaxAllocHeapBytes = ESP.getMaxAllocHeap();
        const uint32_t wMinFreeHeapBytes = ESP.getMinFreeHeap();

        const std::array<uint32_t, 6> wDeviceMetrics = {
            wPartitionSize, wSketchSize, wHeapSize, wFreeHeap,
            wMaxAllocHeapBytes, wMinFreeHeapBytes
        };

        /* Reformat and update the device information only when a raw metric changes. */
        if (!wDeviceInfoInitialized || (wDeviceMetrics != wPreviousDeviceMetrics))
        {
            wDeviceInfoInitialized = true;

            /* Update previous device metrics for future comparisons */
            wPreviousDeviceMetrics = wDeviceMetrics;

            String wFlashUsage = "Unavailable";
            if (wPartitionSize > 0)
            {
                /* Calculate flash usage percentage */
                wFlashUsage = String(wSketchSize * 100.0f / wPartitionSize, 2) + "%";
            }

            String wHeapUsage = "Unavailable";
            if ((wHeapSize > 0) && (wFreeHeap <= wHeapSize))
            {
                /* Calculate heap usage percentage */
                wHeapUsage = String((wHeapSize - wFreeHeap) * 100.0f / wHeapSize, 2) + "%";
            }

            /* Convert maximum allocatable and minimum free heap to kilobytes. */
            String wMaxAllocHeap = String(wMaxAllocHeapBytes / 1000.0f, 2) + " kB";
            String wMinFreeHeap  = String(wMinFreeHeapBytes  / 1000.0f, 2) + " kB";

            const std::array<const char*, 4> wDeviceInfoNames = {
            // "Flash",            "Heap",           "Max block",        "Min free"
                "ESP Flash usage", "ESP Heap usage", "ESP MaxAllocHeap", "ESP MinFreeHeap"
            };

            ESPUI.updateLabel(mWebUIControlID.mDeviceInfoLabel,
                BuildHtmlTable(wDeviceInfoNames, {
                    wFlashUsage.c_str(), wHeapUsage.c_str(),
                    wMaxAllocHeap.c_str(), wMinFreeHeap.c_str()}));
        }
    }

}

/** @brief Control callback function for handling web UI events
 *
 * @details
 * This function is called whenever a control event occurs in the web UI.
 * It processes the event, extracts relevant information, and sends it to
 * the control event queue for further handling.
 *
 * @param apSender Pointer to the control that triggered the event
 * @param aType The type of event (e.g., value changed, button pressed)
 * @param apParam Pointer to additional parameters associated with the event
 */
void WebSite::ControlCallback(BasicControl* apSender, int aType , void* apParam)
{
    /* Check if the website instances and inputs are valid */
    if (mpWebSiteInstance &&                                // Check if the website instance is valid
        mpWebSiteInstance->mpControlEventQueue &&           // Check if the control event queue
        mpWebSiteInstance->mpControlEventNotification &&    //          and control event notification are available
        apSender && apParam)                                // Check if the sender and parameter are valid
    {
        const ControlParam* wParam = static_cast<const ControlParam*>(apParam);
        WebSite* wWebSite = mpWebSiteInstance;

        ControlEvent wEvent{};

        wEvent.mControlId    = apSender->GetId();
        wEvent.mEventType    = aType;
        wEvent.mControlParam = *wParam;

        wEvent.mPayload.mParameter.mValue = static_cast<int32_t>(apSender->getValueInt());

        /* Lambda function to safely copy text into a destination buffer */
        auto wCopyText = [](char* apDestination, size_t aDestinationSize, const String& arText) -> bool
        {
            bool wRetValue = false;

            if (arText.length() < aDestinationSize)
            {
                snprintf(apDestination, aDestinationSize, "%s", arText.c_str());
                wRetValue = true;
            }

            return wRetValue;
        };

        switch (wEvent.mControlParam.mHandler)
        {
            /* For controls that require text or color values, copy the value into the event structure */
            case ControlHandler::Text:
            case ControlHandler::Color:
            case ControlHandler::Timer:
                /* Copy the text value from the sender control into the event structure */
                if (!wCopyText(wEvent.mPayload.mParameter.mValueText,
                        sizeof(wEvent.mPayload.mParameter.mValueText), apSender->getValue()))
                {
                    LOG(LOG_ERROR, "WebSite::ControlCallback() Text value too long for control %04X", wEvent.mControlId);
                    /* Set the handler to None to indicate that the event should not be processed further */
                    wEvent.mControlParam.mHandler = ControlHandler::None;
                }
                break;

            /* For WiFi controls, copy the SSID index and password into the event structure */
            case ControlHandler::WifiSsidSelect:
            case ControlHandler::WifiPassword:
            case ControlHandler::WifiPasswordShowHide:
            case ControlHandler::WifiConnectButton:
            case ControlHandler::WifiScanButton:
            case ControlHandler::WifiForgetButton:
            {
                BasicControl* wSsidControl     = ESPUI.getControl(wWebSite->mWebUIControlID.mWifiSSIDs);
                BasicControl* wPasswordControl = ESPUI.getControl(wWebSite->mWebUIControlID.mWifiPassword);

                if (wSsidControl && wPasswordControl)
                {
                        wEvent.mPayload.mWifi.mSsidIndex = static_cast<int32_t>(wSsidControl->getValueInt());

                        if (!wCopyText(wEvent.mPayload.mWifi.mPassword,
                                sizeof(wEvent.mPayload.mWifi.mPassword), wPasswordControl->getValue()))
                    {
                        LOG(LOG_ERROR, "WebSite::ControlCallback() WiFi password exceeds the supported length");
                        /* Set the handler to None to indicate that the event should not be processed further */
                        wEvent.mControlParam.mHandler = ControlHandler::None;
                    }
                }
                else
                {
                    LOG(LOG_ERROR, "WebSite::ControlCallback() WiFi controls are unavailable");
                    /* Set the handler to None to indicate that the event should not be processed further */
                    wEvent.mControlParam.mHandler = ControlHandler::None;
                }
                break;
            }

            /* Handle other control types that do not require additional data copying */
            case ControlHandler::Switcher:
            case ControlHandler::Select:
            case ControlHandler::Slider:
            default:
                // nothing to do here
                break;
        }

        /* Send the control event to the queue if it has a valid handler */
        if (wEvent.mControlParam.mHandler != ControlHandler::None)
        {
            /* Attempt to send the control event to the FreeRTOS queue and notify the task if successful */
            if (mpWebSiteInstance->mpControlEventQueue->add(wEvent, 0) == pdTRUE)
            {
                mpWebSiteInstance->mpControlEventNotification->Notify();
            }
            else
            {
                LOG(LOG_ERROR, "WebSite::ControlCallback() Control event queue is full; dropping control %04X", wEvent.mControlId);
            }
        }
    }
}
