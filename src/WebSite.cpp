/*
 * WebSite.cpp
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#include <iostream>
#include <sstream>
#include <iomanip>

#include "Logger.h"
#include "WebSite.h"


/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)

typedef struct tSetting
{
    std::string name;
    int val;
    int default_val;
    int UI_ID; // for GUI
} tSetting;

const char* WORD_CLOCK_VERSION = "0.0.1";

#define CSS_COLOR_STYLE   "min-height:40px;"

static void ColorChangedCallback(Control* apSender, int aType)
{
//    String tmpColor = apSender->value;
//    tmpColor.remove(0, 1);
//    uint32_t rcvdColor = strtol(tmpColor.c_str(), NULL, 16);

    std::string wColorStr = apSender->value.c_str();
//    Serial.print("Selected color: ");
//    Serial.println(color_str.c_str());

    /* Cut off '#' prefix */
    int wColorVal = std::stoi(wColorStr.substr(1), nullptr, 16);

    ESPUI.updateText(apSender->GetId(), apSender->value);

}

static Control::ControlId_t statusLabelID, statusNightModeID, statusLanguageID, intensity_web_HintID, DayNightSectionID, LEDsettingsSectionID;

static tSetting mSettings[setting_type_count] =
{
    /* colorTime      */ { "colorTime",      {}, 0x00ff00, {} },
    /* colorBack      */ { "colorBack",      {}, 0x000000, {} },
    /* colorHour      */ { "colorHour",      {}, 0xff0000, {} },
    /* colorMin1      */ { "colorMin1",      {}, 0xff0000, {} },
    /* colorMin2      */ { "colorMin2",      {}, 0xffffff, {} },
    /* colorMin3      */ { "colorMin3",      {}, 0x00ff00, {} },
    /* colorMin4      */ { "colorMin4",      {}, 0xffff00, {} },
    /* usenightmode   */ { "usenightmode",   {}, 1,        {} },
    /* day_time_start */ { "day_time_start", {}, 7,        {} },
    /* day_time_stop  */ { "day_time_stop",  {}, 22,       {} }
};


/**
 * @brief Constructor
 */
WebSite::WebSite()
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

void WebSite::Init(void)
{
    /* Register WiFi events listener */
	WiFi.onEvent(
        std::bind(&WebSite::HandleWifiEvent, this, std::placeholders::_1));


    /*******************************************************************
     *                        Setup Web Interface                      *
     *******************************************************************/

    /* ESPUI Log mode */
    ESPUI.setVerbosity(Verbosity::Verbose);

    /* Section General */
//    ESPUI.separator("General:");
    ESPUI.addControl(Control::Type::Separator, "General:", "", Control::Color::Alizarin, Control::noParent);
    // Status label
//    statusLabelID = ESPUI.label("Status:", Control::Color::Dark, "Operational");
    statusLabelID = ESPUI.addControl(Control::Type::Label, "Status:", "Operational", Control::Color::Dark, Control::noParent, nullptr, nullptr);
    // WordClock version:
    //ESPUI.label("Version:", Control::Color::None, WORD_CLOCK_VERSION);
    statusLabelID = ESPUI.addControl(Control::Type::Label, "Version:", WORD_CLOCK_VERSION, Control::Color::Dark, Control::noParent, nullptr, nullptr);

    /* Section LED settings */
    LEDsettingsSectionID = ESPUI.addControl(Control::Type::Separator, "LED settings:", "", Control::Color::Alizarin, Control::noParent);
    // Time color selector:
    SetupColor(setting_type::colorTime, "Time");
    // Background color selector:
    SetupColor(setting_type::colorBack, "Background");




}

void WebSite::Loop(void)
{
    // nothig to do
}


void WebSite::SetupColor(const setting_type aType, const char*  apCaption)
{
    char wHexColor[10];

    /* Color selector */
    //std::ostringstream ss;
    //ss << "#" << std::setfill('0') << std::setw(6) << std::hex << mSettings[aType].val;
    //std::string time_str = ss.str();
    sprintf(wHexColor, "#%06X", static_cast<uint32_t>(mSettings[aType].default_val));

    LOG(LOG_DEBUG, "WebSite::SetupColor() Color %d", static_cast<uint32_t>(mSettings[aType].default_val));

    const auto ID = ESPUI.text(apCaption, /*call_generic_color*/ColorChangedCallback, Control::Color::Dark, /*time_str.c_str()*/wHexColor);
    ESPUI.setElementStyle(ID, CSS_COLOR_STYLE);
    ESPUI.setInputType(ID, "color");

    //all_settings[type].UI_ID = ID;
    //UI2settingMap[ID] = type;
}

void WebSite::HandleWifiEvent(WiFiEvent_t aEvent)
{
	switch (aEvent)
	{
		case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            /* LOG */
            LOG(LOG_DEBUG, "WebSite::HandleWifiEvent() Start web server");
            /* Start web server */
            ESPUI.begin("Wordclock");
            //ESPUI.jsonReload();
			break;

		default:
			break;
    }
}