/*
 * WebSite.cpp
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
//#include <ArduinoJson.h>
//#include <ESPAsyncWebServer.h>
//#include <ESPUI.h>

#include "Logger.h"
#include "WebSite.h"


/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)

#define CUSTOM_CSS "<style>\
  .d30 {\
    box-sizing: border-box;\
    white-space: nowrap;\
    border-radius: 0.2em;\
    padding: 0.12em 0.4em 0.14em;\
    text-align: center;\
    color: #ffffff;\
    font-weight: 700;\
    line-height: 1.3;\
    display: inline-block;\
    white-space: nowrap;\
    vertical-align: baseline;\
    position: relative;\
    top: -0.15em;\
    background-color: #999999;\
    margin-bottom: 10px;\
    white-space: pre-wrap;\
    word-wrap: break-word;\
    background-color: unset;\
    width: 30\%;\
    text-align: left;\
  }\
  .d60 {\
    box-sizing: border-box;\
    white-space: nowrap;\
    border-radius: 0.2em;\
    padding: 0.12em 0.4em 0.14em;\
    text-align: center;\
    color: #ffffff;\
    font-weight: 700;\
    line-height: 1.3;\
    display: inline-block;\
    white-space: nowrap;\
    vertical-align: baseline;\
    position: relative;\
    top: -0.15em;\
    background-color: #999999;\
    margin-bottom: 10px;\
    white-space: pre-wrap;\
    word-wrap: break-word;\
    background-color: unset;\
    width: 60\%;\
    text-align: left;\
  }\
  .v30 {\
    box-sizing: border-box;\
    white-space: nowrap;\
    border-radius: 0.2em;\
    padding: 0.12em 0.4em 0.14em;\
    text-align: center;\
    color: #ffffff;\
    font-weight: 700;\
    line-height: 1.3;\
    display: inline-block;\
    white-space: nowrap;\
    vertical-align: baseline;\
    position: relative;\
    top: -0.15em;\
    background-color: #999999;\
    margin-bottom: 10px;\
    white-space: pre-wrap;\
    word-wrap: break-word;\
    width: 30\%;\
  }\
  .v40 {\
    box-sizing: border-box;\
    white-space: nowrap;\
    border-radius: 0.2em;\
    padding: 0.12em 0.4em 0.14em;\
    text-align: center;\
    color: #ffffff;\
    font-weight: 700;\
    line-height: 1.3;\
    display: inline-block;\
    white-space: nowrap;\
    vertical-align: baseline;\
    position: relative;\
    top: -0.15em;\
    background-color: #999999;\
    margin-bottom: 10px;\
    white-space: pre-wrap;\
    word-wrap: break-word;\
    width: 40\%;\
  }\
  .v70 {\
    box-sizing: border-box;\
    white-space: nowrap;\
    border-radius: 0.2em;\
    padding: 0.12em 0.4em 0.14em;\
    text-align: center;\
    color: #ffffff;\
    font-weight: 700;\
    line-height: 1.3;\
    display: inline-block;\
    white-space: nowrap;\
    vertical-align: baseline;\
    position: relative;\
    top: -0.15em;\
    background-color: #999999;\
    margin-bottom: 10px;\
    white-space: pre-wrap;\
    word-wrap: break-word;\
    width: 70\%;\
  }\
  .u10 {\
    box-sizing: border-box;\
    white-space: nowrap;\
    border-radius: 0.2em;\
    padding: 0.12em 0.4em 0.14em;\
    text-align: center;\
    color: #ffffff;\
    font-weight: 700;\
    line-height: 1.3;\
    display: inline-block;\
    white-space: nowrap;\
    vertical-align: baseline;\
    position: relative;\
    top: -0.15em;\
    background-color: #999999;\
    margin-bottom: 10px;\
    white-space: pre-wrap;\
    word-wrap: break-word;\
    width: 10\%;\
    background-color: unset;\
    text-align: left;\
  }\
  a {color: #ffffff;}\
 </style>"

#define LABLE_STYLE_GROUP               "background-color: unset; width: 100%; text-align: center;"
#define LABLE_STYLE_CLEAR               "background-color: unset; width: 60%; text-align: left;"
#define LABLE_STYLE_VALUE               "width: 30%;"
#define LABLE_STYLE_UNIT                "background-color: unset; width: 10%; text-align: left;"
#define LABLE_STYLE_DASH                "background-color: unset; width: 100%; font-size: 40px"
#define LABLE_STYLE_DESCRIPTION         "background-color: unset; width: 100%; text-align: center; font-weight: normal;"
#define LABLE_STYLE_INPUT_LABEL         "background-color: unset; width: 30%; text-align: left;"
#define LABLE_STYLE_INPUT_TEXT          "width: 65%; color: black"
#define LABLE_STYLE_NUMER_LABEL         "background-color: unset; width: 74%; text-align: left;"
#define LABLE_STYLE_SWITCH_LABEL        "background-color: unset; width: 83%; text-align: left;"

/* Initialize the private static pointer */
WebSite* WebSite::mpWebSite = nullptr;


/**
 * @brief Safe strcat function
 *
 * @param   dest string destination
 * @param   src string source
 * @param   dest_size destination size
 */
static char* strcat_safe(char *dest, const char *src, size_t dest_size)
{
    size_t dest_len = strlen(dest);
    size_t src_len  = strlen(src);

    if ((dest_len + src_len) >= dest_size)
    {
        // not enough space
        return nullptr;
    }

    strcat(dest, src);
    return dest;
}

static void GeneralCallback(Control* apSender, int aType)
{

}


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
    /* Register WiFi events listener */
	WiFi.onEvent(
        std::bind(&WebSite::HandleWifiEvent, this, std::placeholders::_1));

    /* Add additional CSS styles to a hidden label */
//    ESPUI.setPanelStyle(ESPUI.label("Refresh your browser if you read this", Control::Color::None, CUSTOM_CSS), "display: none");

    /* ESPUI Log mode */
    ESPUI.setVerbosity(Verbosity::Verbose);

    /* System tab */
    uint16_t wIdTabSystem = ESPUI.addControl(
            Control::Type::Tab, "", /*webText.SYSTEM[config.lang]*/ "System", Control::Color::None, 0, GeneralCallback);

    auto wWiFiGroup = AddGroupHelper(/*webText.WIFI_INFO[config.lang]*/ "WiFi-Informations", Control::Color::Dark, wIdTabSystem);
//    //uint16_t wIdTables.system_wifi = addEmtyElement(wiFiGroup);

    ESPUI.addControl(Control::Type::Separator, "", "", Control::Color::None, wIdTabSystem);

    /* Date & Time */
    auto wDateTimeGroup = AddGroupHelper(/*webText.DATETIME[config.lang]*/ "/setvalue/setdatetime", Control::Color::Dark, wIdTabSystem);

    // NTP Date&Time
    uint16_t wIdSysDate  = ESPUI.addControl(Control::Type::Label, "", "", Control::Color::None, wDateTimeGroup); // control: output date

    ESPUI.setElementStyle(wIdSysDate, "width: 25%; font-size: 20px");
    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, "", " ", Control::Color::None, wIdSysDate), "background-color: unset; width: 5%"); // spacer

    uint16_t wIdSysTime  = ESPUI.addControl(Control::Type::Label, "", "", Control::Color::None, wIdSysDate); // control: output time

    ESPUI.setElementStyle(wIdSysTime, "width: 25%; font-size: 20px");
    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, "", " ", Control::Color::None, wIdSysDate), "background-color: unset; width: 5%"); // spacer

    uint16_t  wIdSsysNtpBtn = ESPUI.addControl(Control::Type::Button, "", /*webText.BUTTON_NTP[config.lang]*/ "Set NTP", Control::Color::Dark, wIdSysDate, GeneralCallback); // control: button

    ESPUI.setElementStyle(wIdSsysNtpBtn, "width: 30%");
    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, "", " ", Control::Color::None, wIdSysDate), "background-color: unset; width: 100%"); // spacer

    // Manual Date&Time
    uint16_t wIdSysDateInput = ESPUI.addControl(Control::Type::Text, "", "", Control::Color::Dark, wDateTimeGroup, GeneralCallback); // control: input date

    ESPUI.setInputType(wIdSysDateInput, "date"); // input control type: date
    ESPUI.setElementStyle(wIdSysDateInput, "width: 25%; color: black; font-size: 20px");
    ESPUI.setElementStyle(ESPUI.addControl(Control::Type::Label, "", " ", Control::Color::None, wDateTimeGroup), "background-color: unset; width: 5%"); // spacer

    uint16_t wIdSysTimeInput = ESPUI.addControl(Control::Type::Text, "", "", Control::Color::Dark, wDateTimeGroup, GeneralCallback); // control: input time

    ESPUI.setInputType(wIdSysTimeInput, "time"); // input control type: time
    ESPUI.setElementStyle(wIdSysTimeInput, "width: 25%; color: black; font-size: 20px");
    ESPUI.setElementStyle(
            ESPUI.addControl(Control::Type::Label, "", " ", Control::Color::None, wDateTimeGroup), "background-color: unset; width: 5%"); // spacer

    uint16_t wIdSysDtiBtn = ESPUI.addControl(Control::Type::Button, "", /*webText.BUTTON_DTI[config.lang]*/ "set manually", Control::Color::Dark, wDateTimeGroup, GeneralCallback); // control: button
    ESPUI.setElementStyle(wIdSysDtiBtn, "width: 30%");

}

void WebSite::Loop(void)
{
}

uint16_t WebSite::AddGroupHelper(const char * apTitle, Control::Color aColor, uint16_t aTab)
{
	auto wGroupID = ESPUI.addControl(Control::Type::Label, apTitle, "", aColor, aTab);
	ESPUI.setElementStyle(wGroupID, LABLE_STYLE_GROUP);

    return wGroupID;
}

uint16_t WebSite::AddGroupValueHelper(const char * apTitle, String aValue, String aUnit, uint16_t aGroup)
{
    ESPUI.setElementStyle(
            ESPUI.addControl(Control::Type::Label, "", apTitle, Control::Color::None, aGroup),
            LABLE_STYLE_CLEAR);

	uint16_t wValueId = ESPUI.addControl(Control::Type::Label, "", aValue, Control::Color::None, aGroup);

    ESPUI.setElementStyle(wValueId, LABLE_STYLE_VALUE);

    ESPUI.setElementStyle(
            ESPUI.addControl(Control::Type::Label, "", aUnit, Control::Color::None, aGroup),
            LABLE_STYLE_UNIT);

    return wValueId;
}

uint16_t WebSite::AddTextInputHelper(const char * apTitle, uint16_t aGroup)
{
    ESPUI.setElementStyle(
            ESPUI.addControl(Control::Type::Label, "", apTitle, Control::Color::None, aGroup), LABLE_STYLE_INPUT_LABEL);

    uint16_t wValueId = ESPUI.addControl(Control::Type::Text, "", "", Control::Color::Dark, aGroup, GeneralCallback);

    ESPUI.setElementStyle(wValueId, LABLE_STYLE_INPUT_TEXT);

    return wValueId;
}

uint16_t WebSite::AddNumberInputHelper(const char * apTitle, uint16_t aGroup)
{
    ESPUI.setElementStyle(
            ESPUI.addControl(Control::Type::Label, "", apTitle, Control::Color::None, aGroup), LABLE_STYLE_NUMER_LABEL);

    uint16_t wValueId = ESPUI.addControl(Control::Type::Number, "", "0", Control::Color::Dark, aGroup, GeneralCallback);

    return wValueId;
}

uint16_t WebSite::AddSwitcherInputHelper(const char * apTitle, uint16_t aGroup)
{
    ESPUI.setElementStyle(
            ESPUI.addControl(Control::Type::Label, "", apTitle, Control::Color::None, aGroup), LABLE_STYLE_SWITCH_LABEL);

    uint16_t wValueId = ESPUI.addControl(Control::Type::Switcher, "", "", Control::Color::Dark, aGroup, GeneralCallback);

    return wValueId;
}

const char* WebSite::OnOffString(uint8_t aValue)
{
    static char wRetStr[64];

    if (aValue != 0)
        snprintf(wRetStr, sizeof(wRetStr), "%s", /*webText.ON[config.lang]*/   "On");
    else
        snprintf(wRetStr, sizeof(wRetStr), "%s", /*webText.OFF[config.lang]*/  "Off");

    return wRetStr;
}

const char* WebSite::ErrOkString(uint8_t aValue)
{
    static char wRetStr[64];

    if (aValue!=0)
        snprintf(wRetStr, sizeof(wRetStr), "%s", /*webText.ERROR[config.lang]*/ "Error");
    else
        snprintf(wRetStr, sizeof(wRetStr), "%s", /*webText.OK[config.lang]*/    "Ok");

    return wRetStr;
}

void WebSite::AddElement(const char* apLabel, const char* apValue)
{
    snprintf(mElementString, sizeof(mElementString),
            "<span class=\"d60\">%s</span><span class=\"v40\">%s</span>",
            apLabel, apValue);

    strcat_safe(mElementBuffer, mElementString, sizeof(mElementBuffer));
}

void WebSite::AddElementUnit(const char* apLabel, const char* apValue, const char* apUnit)
{
    snprintf(mElementString, sizeof(mElementString),
            "<span class=\"d60\">%s</span><span class=\"v30\">%s</span><span class=\"u10\">%s</span>",
            apLabel, apValue, apUnit);

    strcat_safe(mElementBuffer, mElementString, sizeof(mElementBuffer));
}

void WebSite::AddElementWide(const char* apLabel, const char* apValue)
{
    snprintf(mElementString, sizeof(mElementString),
            "<span class=\"d30\">%s</span><span class=\"v70\">%s</span>",
            apLabel, apValue);

    strcat_safe(mElementBuffer, mElementString, sizeof(mElementBuffer));
}

uint16_t WebSite::AddEmtyElement(const uint16_t aParent)
{
    uint16_t wId = ESPUI.addControl(Control::Type::Label, "", "--", Control::Color::None, aParent);
    ESPUI.setElementStyle(wId, "background-color: unset; width: 100%");

    return wId;
}

const char* WebSite::AddUnit(const char* apInput, const char* apUnit)
{
    static char wRetStr[128];

    snprintf(wRetStr, sizeof(wRetStr), "%s %s", apInput, apUnit);

    return wRetStr;
}

void WebSite::UpdateElements(uint16_t aParent)
{
    ESPUI.updateLabel(aParent, mElementBuffer);
}

void WebSite::HandleWifiEvent(WiFiEvent_t aEvent)
{
	switch (aEvent)
	{
		case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            ESPUI.begin("WordClock");
			break;

		default:
			break;
    }
}