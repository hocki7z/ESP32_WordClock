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

#define LABLE_STYLE_GROUP               "background-color: unset; width: 100%; text-align: center;"
#define LABLE_STYLE_CLEAR               "background-color: unset; width: 60%; text-align: left;"
#define LABLE_STYLE_VALUE               "width: 30%;"
#define LABLE_STYLE_UNIT                "background-color: unset; width: 10%; text-align: left;"
#define LABLE_STYLE_DASH                "background-color: unset; width: 100%; font-size: 40px"
#define LABLE_STYLE_DESCRIPTION         "background-color: unset; width: 100%; text-align: center; font-weight: normal;"
#define LABLE_STYLE_NUMER_LABEL         "background-color: unset; width: 74%; text-align: left;"
#define LABLE_STYLE_SWITCH_LABEL        "background-color: unset; width: 75%; text-align: left;"


// Color selection from https://encycolorpedia.com/aaaaaa
//     #aaaaaa #aaa- RGB(170, 170, 170) - Greyscale
//     #d4d4d4 - 25% lighter than Greyscale
//     #c0c0c0 - Silver (web save)
//
//     #dfe2eb - 25% lighter than #c0c0c0 - Light Grey

#define ELEMENT_STYLE_BUTTON            "width: 95%;" // margin-bottom: 10px;
#define ELEMENT_STYLE_SEPARATOR         "background-color: unset; width: 95%; border-bottom: 1px solid #dfe2eb; height: 1px; margin: 4px 0;"
#define ELEMENT_STYLE_SPACER_TEMPLATE   "background-color: unset; width: 100%; height: %upx;"  // %upx will be replaced by the height in pixels

#define ELEMENT_STYLE_TEXT_LABEL        "background-color: unset; width: 30%; text-align: left;"
#define ELEMENT_STYLE_TEXT_INPUT        "width: 65%; color: black;"
#define ELEMENT_STYLE_SLIDER_LABEL      "background-color: unset; width: 30%; text-align: left;"
#define ELEMENT_STYLE_SLIDER_INPUT      "width: 55%; color: black;"
#define ELEMENT_STYLE_SWITCHER_LABEL    "background-color: unset; width: 80%; text-align: left;"
#define ELEMENT_STYLE_SWITCHER_INPUT    "align-self: bottom;"

#define STYLE_HIDDEN                    "background-color: unset; width: 0px; height: 0px; display: none;"
#define STYLE_NUM_TEMP_ADJUST_NORMAL    "width: 16%; color: black; background: rgba(255,255,255,0.8);"
#define STYLE_NUM_POWER_ADJUST_NORMAL   "width: 16%; color: black; background: rgba(255,255,255,0.8);"
#define STYLE_NUM_POWER_ADJUST_ERROR    "width: 16%; color: black; background: rgba(231,76,60,0.8);"
#define STYLE_NUM_POWER_ADJUST_DISABLED "width: 16%; color: black; background: rgba(153,153,153,0.8);"
#define STYLE_LBL_ADJUST                "background-color: unset; width: 84%; text-align-last: left;"
#define STYLE_LBL_INOUT                 "width: 16%;"
#define STYLE_LBL_API                   "width: 29%;"
#define STYLE_SWITCH_INOUT              "margin-bottom: 10px; width: 12.5%; vertical-align: middle; "
#define STYLE_NUM_INOUT_MANUAL_INPUT    "width: 16%; color: black; background: rgba(255,255,255,0.8);"
#define STYLE_LBL_API_VALUE_OUTPUT      "background-color: unset; text-align: left; width: 70.5%;"
#define STYLE_LBL_INOUT_VALUE_OUTPUT    "background-color: unset; text-align: left; width: 83.5%;"
#define STYLE_LBL_INOUT_MANUAL_ENABLE   "background-color: unset; text-align: left; width: 70%; vertical-align: bottom; margin-bottom: 5px"


#define CSS_SHOWBUTTON_STYLE            "font-size:1.6rem;min-height:4rem;padding:1rem;margin:3px;border:2px solid #404040;border-radius:5px;"
#define CSS_SHOWBUTTON_DW_STYLE         String(CSS_SHOWBUTTON_STYLE) + "color:#999;background-color: #222;"
#define CSS_SHOWBUTTON_UP_STYLE         String(CSS_SHOWBUTTON_STYLE) + "color:#222;background-color: #bbb;"
#define CSS_TEXT_STYLE                  "height:26px;width:180px;"
#define CSS_TEXT_LABEL_STYLE            "margin-bottom: 0;font-size: 1.2em; background-color: unset; vertical-align: middle; text-align: left; width:55%"
#define CSS_SELECT_STYLE                "width:60px;"
#define CSS_SELECT_LABEL_STYLE          "font-size: 1.2em; background-color: unset; vertical-align: bottom; text-align: left; width:80%;margin-bottom: 3;"
#define CSS_NUMBER_STYLE                "width:60px;"
#define CSS_NUMBER_LABEL_STYLE          "font-size: 1.2em; background-color: unset; vertical-align: middle; text-align: left; width:80%"
#define CSS_STATUS_STYLE                "font-size: 1.6em;font-weight:400;font-variant-caps: small-caps; background-color: unset; color: #eee; "
#define CSS_STATUS_PANEL_STYLE          "margin-left: auto; margin-right: auto;float: none;"
#define CSS_ALARM_PANEL_STYLE            "color:red;"
#define CSS_COLOR_STYLE                 "min-height:40px;"
#define CSS_CONTROL_LABEL_STYLE         "align-items: center; font-size: 1.2em; background-color: unset; vertical-align: middle; text-align: left; width:80%"
#define CSS_CONTROL_SWITCH_STYLE        "align-items: center; text-align: right;"
#define CSS_SLIDER_LABEL_STYLE          "font-size:1.2rem;font-weight:400;width:18%; background-color: unset;"
#define CSS_SAVEBUTTON_STYLE            "margin:0 2rem 1rem;width:200px;outline:0; border:0; border-radius:4px; border-bottom:4px solid #2b6cb0;background-color:#4299e1; color:white;"
#define CSS_SAVEBUTTON_DOWN_STYLE       "border-bottom:4px solid #E91E63; background-color:#3F51b5;"
#define CSS_NO_BG_STYLE                 "background-color: unset;"
#define CSS_VERYSPEC_STYLE              "background: linear-gradient(170deg, rgba(22,22,222,1) 0%, rgba(22,22,222,1) 33%, white 50%, rgba(176,176,0,1) 66%, rgba(176,176,0,1) 100%); border-bottom: #000;"


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
#if LOG_LEVEL >= LOG_DEBUG    
    ESPUI.setVerbosity(Verbosity::Verbose);
#else
    /* Turn off verbose debugging */
    ESPUI.setVerbosity(Verbosity::Quiet);
#endif /* LOG_LEVEL >= LOG_DEBUG */

	/* Make sliders continually report their position as they are being dragged. */
	ESPUI.sliderContinuous = true;

	auto maintab = ESPUI.addControl(Control::Type::Tab, "", "Basic controls");

	ESPUI.addControl(Control::Type::Separator, "General controls", "", Control::Color::None, maintab);
	ESPUI.addControl(Control::Type::Button, "Button", "Button 1", Control::Color::Alizarin, maintab, nullptr);

	auto mainLabel = ESPUI.addControl(Control::Type::Label, "Label", "Label text", Control::Color::Emerald, maintab, nullptr);
	auto mainSwitcher = ESPUI.addControl(Control::Type::Switcher, "Switcher", "", Control::Color::Sunflower, maintab, nullptr);

	//Sliders default to being 0 to 100, but if you want different limits you can add a Min and Max control
	auto mainSlider = ESPUI.addControl(Control::Type::Slider, "Slider", "200", Control::Color::Turquoise, maintab, nullptr);
	ESPUI.addControl(Control::Type::Min, "", "10", Control::Color::None, mainSlider);
	ESPUI.addControl(Control::Type::Max, "", "400", Control::Color::None, mainSlider);

	//Number inputs also accept Min and Max components, but you should still validate the values.
	auto mainNumber = ESPUI.addControl(Control::Type::Number, "Number Input", "50", Control::Color::Emerald, maintab, nullptr);
	ESPUI.addControl(Control::Type::Min, "",   "0", Control::Color::None, mainNumber);
	ESPUI.addControl(Control::Type::Max, "", "100", Control::Color::None, mainNumber);    

    /***************************************************************************************************************************/

    // Example from:
    //      https://github.com/ChrSchu90/PlatformIo.TCapChamp
    auto _tab = ESPUI.addControl(Control::Type::Tab, emptyString.c_str(), "Example 1");

    // WiFi configuration group
    auto lblWifiSettings = ESPUI.addControl(Control::Type::Label, "WiFi Configuration", emptyString, Control::Color::None, _tab);
    ESPUI.setElementStyle(lblWifiSettings, STYLE_HIDDEN);

    auto _Ssid  = ESPUI.addControl(Control::Type::Select, emptyString.c_str(), "SSID1", Control::Color::None, lblWifiSettings);
    ESPUI.setEnabled(_Ssid, false);
    auto _Passw = ESPUI.addControl(Control::Type::Text,   emptyString.c_str(), "password", Control::Color::None, lblWifiSettings);
    ESPUI.setInputType(_Passw, "password");

    auto _ShowHidePassw = ESPUI.addControl(Control::Type::Switcher, emptyString.c_str(), String(0), Control::Color::None, lblWifiSettings);
    ESPUI.setElementStyle(_ShowHidePassw, STYLE_SWITCH_INOUT);
    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, emptyString.c_str(), "Show/Hide Password", Control::Color::None, lblWifiSettings), STYLE_LBL_INOUT_MANUAL_ENABLE);

    auto _btnSave = ESPUI.addControl(Control::Type::Button, emptyString.c_str(), "Save & Connect", Control::Color::None, lblWifiSettings);
    auto _btnScan = ESPUI.addControl(Control::Type::Button, emptyString.c_str(), "Search for WiFi", Control::Color::None, lblWifiSettings);

    // Device info group
    auto device = String("Model:\t ") + String(ESP.getChipModel()) + "\n" +
                "Flash:\t " + String(ESP.getFlashChipSize()) + "\n" +
                "Freq:\t " + String(ESP.getCpuFreqMHz()) + " MHz\n" +
                "Cores:\t " + String(ESP.getChipCores()) + "\n" +
                "Revision: " + String(ESP.getChipRevision());
    auto lblDevice = ESPUI.addControl(Control::Type::Label, "Device", device, Control::Color::None, _tab);
    ESPUI.setElementStyle(lblDevice, "background-color: unset; text-align-last: left;");

    // Performance group
    auto _lblPerformance = ESPUI.addControl(Control::Type::Label, "Performance", emptyString, Control::Color::None, _tab);
    ESPUI.setElementStyle(_lblPerformance, "background-color: unset; text-align-last: left;");

    auto currentMillis  = esp_timer_get_time() / 1000;
    auto seconds        = currentMillis / 1000;
    auto minutes        = seconds / 60;
    auto hours          = minutes / 60;
    auto days           = hours / 24;
    currentMillis       %= 1000;
    seconds             %= 60;
    minutes             %= 60;
    hours               %= 24;
    float freeHeap      = ESP.getFreeHeap();
    uint32_t heapSize   = ESP.getHeapSize();
    float usedHeap      = heapSize - freeHeap;
    float maxUsedHeap   = ESP.getMaxAllocHeap();
    float freeSketch    = ESP.getFreeSketchSpace();
    uint32_t sketchSize = freeSketch + ESP.getSketchSize();

    auto performance = String("Uptime:\t\t\t\t") + String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s\n" +
                "Heap Usage:\t\t\t"     + String(usedHeap, 0) + "/" + String(heapSize) + " (" + String(usedHeap / heapSize * 100.0f, 2) + " %)\n" +
                "Heap Allocated Max:\t" + String(maxUsedHeap, 0) + " (" + String(maxUsedHeap / heapSize * 100.0f, 2) + " %)\n" +
                "Sketch Used:\t\t\t"    + String(sketchSize - freeSketch, 0) + "/" + String(sketchSize) + " (" + String(freeSketch / sketchSize * 100.0f, 2) + " %)\n" +
                "Temperature:\t\t\t"    + String(temperatureRead(), 1) + " °C";
    ESPUI.updateLabel(_lblPerformance, performance);

    // Temperature Adjustment group
    auto tmpAdjGrp = ESPUI.addControl(Control::Type::Label, "Temperature Adjustment", emptyString, Control::Color::None, _tab);
    ESPUI.setElementStyle(tmpAdjGrp, STYLE_HIDDEN);
    for (size_t i = 0; i < 10; i++)
    {
        auto numAdjTemp = ESPUI.addControl(Control::Type::Number, emptyString.c_str(), String(i, 1), Control::Color::None, tmpAdjGrp);
        ESPUI.setElementStyle(numAdjTemp, STYLE_NUM_TEMP_ADJUST_NORMAL);
        ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, emptyString.c_str(), "°C offset at " + String(25) + " °C", Control::Color::None, tmpAdjGrp), STYLE_LBL_ADJUST);
    }


    
    /***************************************************************************************************************************/


    // Example from:
    //      https://github.com/BlockThor/ESPUI-web-interace-for-WS2812FX-with-AccessPoint
    uint16_t tabSettings = ESPUI.addControl(Control::Type::Tab, "", "Example 2", Control::Color::None, Control::noParent);
    ESPUI.setElementStyle(tabSettings, "color: red;");
    ESPUI.setPanelStyle(tabSettings, "color: blue;");

    auto wifiap_web = ESPUI.addControl(Control::Type::Label, "WiFi Access Point Credentials", "", Control::Color::Wetasphalt, tabSettings);
    ESPUI.setElementStyle(wifiap_web, "background-color: unset;");

    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, "", "SSID ", Control::Color::Dark, wifiap_web), CSS_TEXT_LABEL_STYLE);
    auto wifiap_ssid_text = ESPUI.addControl(Control::Type::Text, "", "SSID1", Control::Color::Wetasphalt, wifiap_web);
    ESPUI.setElementStyle(wifiap_ssid_text, CSS_TEXT_STYLE);
    ESPUI.addControl(Control::Type::Max, "", "20", Control::Color::None, wifiap_ssid_text);

    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, "", "Password AP", Control::Color::Wetasphalt, wifiap_web), CSS_TEXT_LABEL_STYLE);
    auto wifiap_pass_text = ESPUI.addControl(Control::Type::Text, "", "password", Control::Color::Wetasphalt, wifiap_web);
    ESPUI.setElementStyle(wifiap_pass_text, CSS_TEXT_STYLE);
    ESPUI.addControl(Control::Type::Max, "", "20", Control::Color::None, wifiap_pass_text);

    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Button, "", "Save & Apply", Control::Color::Dark, wifiap_web), CSS_SAVEBUTTON_STYLE);

    // Speed slider
    auto speedSlider = ESPUI.addControl(Control::Type::Slider, "Speed", String(50), Control::Color::Wetasphalt, tabSettings);
    ESPUI.addControl(Control::Type::Min, "", "0", Control::Color::None, speedSlider);
    ESPUI.addControl(Control::Type::Max, "", "100", Control::Color::None, speedSlider);
    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, "", "Faster", Control::Color::None, speedSlider), CSS_SLIDER_LABEL_STYLE);
    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, "", "Fast", Control::Color::None, speedSlider), CSS_SLIDER_LABEL_STYLE);
    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, "", "Norm", Control::Color::None, speedSlider), CSS_SLIDER_LABEL_STYLE);
    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, "", "Slow", Control::Color::None, speedSlider), CSS_SLIDER_LABEL_STYLE);
    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, "", "Slower", Control::Color::None, speedSlider), CSS_SLIDER_LABEL_STYLE);
    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, "", " ", Control::Color::None, speedSlider), "font-size:1.2rem;font-weight:400;width:18%; background-color: unset;width:8%;"); //String(CSS_SLIDER_LABEL_STYLE) + "width:8%;");



    return;

    ESPUI.addControl(Control::Type::Tab, "", "Wordclock", Control::Color::None, Control::noParent);

    mWebUIControlID.mSettingsTab = ESPUI.addControl(Control::Type::Tab, "Settings", "Settings", Control::Color::None, Control::noParent);


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
    mWebUIControlID.mSettingsLedGroup = AddGroupHelper("LED", mWebUIControlID.mSettingsTab, Control::Color::None);
    /* Slider for LED brightness selection */
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, ELEMENT_STYLE_SLIDER_LABEL, "Brightness");
    mWebUIControlID.mDisplayLedBrightness = AddPercentageSliderControl("", mWebUIControlID.mSettingsLedGroup, ELEMENT_STYLE_SLIDER_INPUT,
            ConfigNS::mKeyDisplayLedBrightness, ConfigNS::mDefaultDisplayLedBrightness);

    /* Switcher for day/night mode activation */
    AddSeparatorElement(mWebUIControlID.mSettingsLedGroup);
    AddSpacerElement(mWebUIControlID.mSettingsLedGroup);
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, ELEMENT_STYLE_SWITCHER_LABEL, "Use day/night mode");
    mWebUIControlID.mDisplayUseNightMode = AddSwitcherControl("", mWebUIControlID.mSettingsLedGroup, ELEMENT_STYLE_SWITCHER_INPUT,
            ConfigNS::mKeyDisplayUseNightMode, ConfigNS::mDefaultDisplayUseNightMode);
    /* Slider for night brightness selection */
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, ELEMENT_STYLE_SLIDER_LABEL, "Night mode brightness");
    mWebUIControlID.mDisplayBrightnessNightMode = AddPercentageSliderControl("", mWebUIControlID.mSettingsLedGroup, ELEMENT_STYLE_SLIDER_INPUT,
            ConfigNS::mKeyDisplayBrightnessNightMode, ConfigNS::mDefaultDisplayBrightnessNightMode);
    /* Time input for night mode start */
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, ELEMENT_STYLE_TEXT_LABEL, "Night mode start time");
    mWebUIControlID.mDisplayNightModeStartTime = AddTimeControl("", mWebUIControlID.mSettingsLedGroup, ELEMENT_STYLE_TEXT_INPUT,
            ConfigNS::mKeyDisplayNightModeStartTime, ConfigNS::mDefaultDisplayNightModeStartTime);
    /* Time input for night mode end */
    AddLabelControl("", mWebUIControlID.mSettingsLedGroup, ELEMENT_STYLE_TEXT_LABEL, "Night mode end time");
    mWebUIControlID.mDisplayNightModeEndTime = AddTimeControl("", mWebUIControlID.mSettingsLedGroup, ELEMENT_STYLE_TEXT_INPUT,
            ConfigNS::mKeyDisplayNightModeEndTime, ConfigNS::mDefaultDisplayNightModeEndTime);


    /* Section DateTime settings */
    mWebUIControlID.mSettingsTimeGroup = AddGroupHelper("Time", mWebUIControlID.mSettingsTab, Control::Color::None);

    /* NTP server selection */
    AddLabelControl("", mWebUIControlID.mSettingsTimeGroup, ELEMENT_STYLE_TEXT_LABEL, "NTP Server");
    mWebUIControlID.mDatetimeNtpServer = AddSelectControl("", mWebUIControlID.mSettingsTimeGroup, ELEMENT_STYLE_TEXT_INPUT,
            ConfigNS::mcNtpServerItems, ConfigNS::mcNtpServerItemsCount,
            ConfigNS::mKeyNtpServer, ConfigNS::mDefaultNtpServer);
    /* Timezone selection */
    AddLabelControl("", mWebUIControlID.mSettingsTimeGroup, ELEMENT_STYLE_TEXT_LABEL, "Time Zone");
    mWebUIControlID.mDatetimeTimeZone = AddSelectControl("", mWebUIControlID.mSettingsTimeGroup, ELEMENT_STYLE_TEXT_INPUT,
            ConfigNS::mcTimezoneNames, ConfigNS::mcTimezoneItemsCount,
            ConfigNS::mKeyTimeZone, ConfigNS::mDefaultTimeZone);

    /* Section WiFi settings */
    mWebUIControlID.mSettingsWiFiGroup = AddGroupHelper("WiFi", mWebUIControlID.mSettingsTab, Control::Color::None);

    /* WiFi SSID selection */
    AddLabelControl("", mWebUIControlID.mSettingsWiFiGroup, ELEMENT_STYLE_TEXT_LABEL, "SSID");
    mWebUIControlID.mWifiSSIDs = AddSelectControl("", mWebUIControlID.mSettingsWiFiGroup, ELEMENT_STYLE_TEXT_INPUT);
    /* WiFi password input */
    AddLabelControl("", mWebUIControlID.mSettingsWiFiGroup, ELEMENT_STYLE_TEXT_LABEL, "Password");
    mWebUIControlID.mWifiPassword = AddPasswordControl("", mWebUIControlID.mSettingsWiFiGroup, ELEMENT_STYLE_TEXT_INPUT);
    /* Show/Hide password switcher */
    AddLabelControl("", mWebUIControlID.mSettingsWiFiGroup, ELEMENT_STYLE_SWITCHER_LABEL, "Show/Hide Password");
    mWebUIControlID.mWifiPasswordShowHide = AddSwitcherControl("", mWebUIControlID.mSettingsWiFiGroup, ELEMENT_STYLE_SWITCHER_INPUT);

    /* WiFi connect button */
    mWebUIControlID.mWifiConnectButton = AddButtonControl("Connect to selected network", mWebUIControlID.mSettingsWiFiGroup, ELEMENT_STYLE_BUTTON);
    /* WiFi scan button */
    AddSeparatorElement(mWebUIControlID.mSettingsWiFiGroup);
    AddSpacerElement(mWebUIControlID.mSettingsWiFiGroup);
    mWebUIControlID.mWifiScanButton = AddButtonControl("Scan WiFi networks", mWebUIControlID.mSettingsWiFiGroup, ELEMENT_STYLE_BUTTON);
    
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

void WebSite::HandleControl(Control* apControl, int aType)
//void WebSite::HandleControl(BasicControl* apControl, int aType)
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
        uint8_t wSsidIndex = static_cast<uint8_t>(std::stoi(ESPUI.getControl(mWebUIControlID.mWifiSSIDs)->getValue().c_str()));

        if (wSsidIndex < mLocalSsidList.size() && (mLocalSsidList[wSsidIndex].mEncrypted))
        {
            String wPassw = ESPUI.getControl(mWebUIControlID.mWifiPassword)->getValue();

            // De-/activate button mWifiConnectButton
            ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton, (wPassw.length() > 0));
        }

        return;
    }
    else if (apControl->GetId() == mWebUIControlID.mWifiPassword)
    {
        /* WiFi password changed */
        String wPassw = ESPUI.getControl(mWebUIControlID.mWifiPassword)->getValue();
        
        // De-/activate button mWifiConnectButton
        ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton, (wPassw.length() > 0));

        return;
    }
    else if (apControl->GetId() == mWebUIControlID.mWifiPasswordShowHide)
    {
        /* WiFi password show/hide switcher changed */
        //bool wShowPassword = ESPUI.getControl(mWebUIControlID.mWifiPasswordShowHide)->value == "true";
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
            uint8_t wSsidIndex = static_cast<uint8_t>(std::stoi(ESPUI.getControl(mWebUIControlID.mWifiSSIDs)->getValue().c_str()));
            String wSsid  = mLocalSsidList[wSsidIndex].mSsid;
            String wPassw = ESPUI.getControl(mWebUIControlID.mWifiPassword)->getValue();

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

Control::ControlId_t WebSite::AddGroupHelper(const char * apLabel, Control::ControlId_t aParent, Control::Color aColor)
{
	Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Label, apLabel, "", aColor, aParent);
	ESPUI.setElementStyle(wControlId, LABLE_STYLE_GROUP);

    return wControlId;
}

Control::ControlId_t WebSite::AddSeparatorElement(Control::ControlId_t aParent)
{
    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Label, "", "", Control::Color::None, aParent);
    ESPUI.setElementStyle(wControlId, ELEMENT_STYLE_SEPARATOR);
    
    return wControlId;
}

Control::ControlId_t WebSite::AddSpacerElement(Control::ControlId_t aParent, uint16_t aHeightPx)
{
    char wStyle[55] = {0}; // 49 (ELEMENT_STYLE_SPACER_TEMPLATE length) + 5 (aHeightPx max length) + 1 (null terminator) = 55 bytes
    sprintf(wStyle, ELEMENT_STYLE_SPACER_TEMPLATE, aHeightPx);

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Label, "", "", Control::Color::None, aParent);
    ESPUI.setElementStyle(wControlId, wStyle);

    return wControlId;
}

Control::ControlId_t WebSite::AddLabelControl(const char* apLabel, Control::ControlId_t aParent, const String& aElementStyle,
        const String& aValue)
{
    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Label, apLabel, aValue, Control::Color::None, aParent);
    ESPUI.setElementStyle(wControlId, aElementStyle.c_str());

    return wControlId;
}

Control::ControlId_t WebSite::AddTextControl(const char* apLabel, Control::ControlId_t aParent, const String& aElementStyle,
        const String& aDefaultValue)
{
    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Text, apLabel,
        aDefaultValue, Control::Color::Dark, aParent, WebSite::ControlCallback);

    ESPUI.setElementStyle(wControlId, aElementStyle.c_str());

    //ESPUI.setInputType(wControlId, "text");

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

    Control::ControlId_t wControlId = AddTextControl(apLabel, aParent, aElementStyle, wHexColor);
    ESPUI.setInputType(wControlId, "color");

    LOG(LOG_DEBUG, "WebSite::AddColorControl() Control %04X, param 0x%08X, color %s",
        wControlId, wColorParam, String(wHexColor).c_str());

    return wControlId;
}

Control::ControlId_t WebSite::AddTimeControl(const char* apTitle, Control::ControlId_t aParent, const String& aElementStyle,
    SettingsNS::tKey aSettingsKey, const uint32_t aDefaultTime)
{
    char wTimeStr[6] = {0};

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        uint32_t wTimeInt = Settings.GetValue<uint32_t>(aSettingsKey, aDefaultTime);
        DateTimeNS::tDateTime wDateTime = DateTimeNS::DwordToDateTime(wTimeInt);

        /* Convert time to string format HH:MM */
        sprintf(wTimeStr, "%02u:%02u", wDateTime.mTime.mHour, wDateTime.mTime.mMinute);
    }

    Control::ControlId_t wControlId = AddTextControl(apTitle, aParent, aElementStyle, wTimeStr);

    ESPUI.setInputType(wControlId, "time");

    LOG(LOG_DEBUG, "WebSite::AddTimeControl() Control %04X, time %s", wControlId, String(wTimeStr).c_str());

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
    bool wState = false;
    
    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wState = Settings.GetValue<bool>(aSettingsKey, aDefaultState);
    }
    else
    {
        wState = aDefaultState;
    }

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Switcher, apLabel, 
        wState ? "1" : "0", Control::Color::Dark, aParent, WebSite::ControlCallback);

    ESPUI.setElementStyle(wControlId, aElementStyle.c_str());

    return wControlId;
}

Control::ControlId_t WebSite::AddSelectControl(const char* apLabel, Control::ControlId_t aParent, const String& aElementStyle,
    const char* const* apItems, uint8_t aItemsCount, SettingsNS::tKey aSettingsKey, const uint8_t aDefaultOption)
{
    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Select, apLabel, apLabel,
        Control::Color::Dark, aParent, WebSite::ControlCallback);

    ESPUI.setElementStyle(wControlId, aElementStyle.c_str());

    if (apItems != nullptr && aItemsCount > 0)
    {
        for (uint8_t wI = 0; wI < aItemsCount; wI++)
        {
            ESPUI.addControl(Control::Type::Option, apItems[wI], String(wI), Control::Color::None, wControlId);
        }
    }

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        uint8_t wSelectedOption = Settings.GetValue<uint8_t>(aSettingsKey, aDefaultOption);
        ESPUI.updateSelect(wControlId, wSelectedOption);
    }

    return wControlId;
}

Control::ControlId_t WebSite::AddPercentageSliderControl(const char* apLabel, Control::ControlId_t aParent, const String& aElementStyle,
    SettingsNS::tKey aSettingsKey, const uint8_t aDefaultValue)
{
    uint8_t wValue;

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wValue = Settings.GetValue<uint8_t>(aSettingsKey, aDefaultValue);
    }
    else
    {
        wValue = aDefaultValue;
    }

    if (wValue > 100)
    {
        wValue = 100;
    }

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Slider, apLabel,
        String(wValue), Control::Color::Dark, aParent, WebSite::ControlCallback);

    ESPUI.setElementStyle(wControlId, aElementStyle.c_str());

    ESPUI.addControl(Control::Type::Min, apLabel, String(  0), Control::Color::None, wControlId);
    ESPUI.addControl(Control::Type::Max, apLabel, String(100), Control::Color::None, wControlId);

    LOG(LOG_DEBUG, "WebSite::AddPercentageSliderControl() Control %04X, value %d", wControlId, wValue);

    return wControlId;
}

Control::ControlId_t WebSite::AddButtonControl(const char* apLabel, Control::ControlId_t aParent, const String& aElementStyle)
{
    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Button, apLabel,
        apLabel, Control::Color::Dark, aParent, WebSite::ControlCallback);

    ESPUI.setElementStyle(wControlId, aElementStyle.c_str());

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

void WebSite::HandleColorControl(Control* aControl, int aType, SettingsNS::tKey aSettingsKey)
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
    uint8_t wSelectedOption = static_cast<uint8_t>(std::stoi(aControl->getValue().c_str()));

    LOG(LOG_DEBUG, "WebSite::HandleSelectControl() Control %04X, new selected option %d",
            aControl->GetId(), wSelectedOption);

    /* Store new selected option in settings */
    Settings.SetValue<uint8_t>(aSettingsKey, wSelectedOption);
}

void WebSite::HandlePercentageSliderControl(Control* aControl, int aType, SettingsNS::tKey aSettingsKey)
{
    /* Retreive new slider value */
    uint8_t wValue = static_cast<uint8_t>(std::stoi(aControl->getValue().c_str()));

    LOG(LOG_DEBUG, "WebSite::HandlePercentageSliderControl() Control %04X, new value %d",
            aControl->GetId(), wValue);

    /* Store new slider value in settings */
    Settings.SetValue<uint8_t>(aSettingsKey, wValue);
}

void WebSite::HandleTimerControl(Control* aControl, int aType, SettingsNS::tKey aSettingsKey)
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
//void WebSite::ControlCallback(Control* apSender, int aType)
{
    if (mpWebSiteInstance)
    {
//        mpWebSiteInstance->HandleControl(apSender, aType);
    }
}
