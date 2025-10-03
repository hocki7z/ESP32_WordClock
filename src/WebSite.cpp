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

static String JSColor(void)
{
    /*
     * Example from: https://jscolor.com/examples/
     */

    String ptr = "";

    ptr += "<script src='https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.5.2/jscolor.min.js'></script>\n";

    ptr += "<p>\n";
    ptr += "Color:\n";
    ptr += "<input data-jscolor='{}' value='#3399FF'>\n";
    ptr += "</p>\n";

    ptr += "<p>\n";
    ptr += "Color with alpha:\n";
    ptr += "<input data-jscolor='{}' value='#3399FF80'>\n";
    ptr += "</p>\n";

    ptr += "<p>\n";
    ptr += "Dark picker + button:\n";
    ptr += "<button data-jscolor='{preset:'dark', width:250, paletteCols:15, value:'rgba(51,153,255,0.5)'}'></button>\n";
    ptr += "</p>\n";

    ptr += "<p>\n";
    ptr += "Custom:\n";
    ptr += "<input data-jscolor='{value:'rgba(51,153,255)', position:'bottom', height:80, backgroundColor:'#333',\n";
    ptr += "    palette:'rgb(0,0,0) #fff #808080 #000 #996e36 #f55525 #ffe438 #88dd20 #22e0cd #269aff #bb1cd4',\n";
    ptr += "    paletteCols:11, hideOnPaletteClick:true}'>\n";
    ptr += "(configured to hide upon clicking bottom palette)\n";
    ptr += "\n";
    ptr += "<script>\n";
    ptr += "// Here we can adjust defaults for all color pickers on page:\n";
    ptr += "jscolor.presets.default = {\n";
    ptr += "    //position: 'right',\n";
    ptr += "    palette: [\n";
    ptr += "        '#000000', '#7d7d7d', '#870014', '#ec1c23', '#ff7e26',\n";
    ptr += "        '#fef100', '#22b14b', '#00a1e7', '#3f47cc', '#a349a4',\n";
    ptr += "        '#ffffff', '#c3c3c3', '#b87957', '#feaec9', '#ffc80d',\n";
    ptr += "        '#eee3af', '#b5e61d', '#99d9ea', '#7092be', '#c8bfe7',\n";
    ptr += "    ],\n";
    ptr += "    //paletteCols: 12,\n";
    ptr += "    //hideOnPaletteClick: true,\n";
    ptr += "};\n";
    ptr += "</script>n";

    return ptr;
}

static String Coloris(void)
{
    /*
     * Example from: https://github.com/mdbassit/Coloris
    *                https://coloris.js.org/
     */

    String ptr = "";

    ptr += "<link rel='stylesheet' href='https://cdn.jsdelivr.net/gh/mdbassit/Coloris@latest/dist/coloris.min.css'/>\n";
    ptr += "<script src='https://cdn.jsdelivr.net/gh/mdbassit/Coloris@latest/dist/coloris.min.js'></script>\n";
    
    ptr += "<p>\n";
    ptr += "Color:\n";
    ptr += "<input type='text' data-coloris>\n";
    ptr += "</p>\n";

    ptr += "<script>\n";
    ptr += "Coloris({                                      \n";
    ptr += "  theme: 'polaroid',                           \n";
    ptr += "  formatToggle: true,                          \n";
    ptr += "  alpha: false,                                \n";
    ptr += "  swatches: [                                  \n";
    ptr += "    'DarkSlateGray',                           \n";
    ptr += "    '#2a9d8f',                                 \n";
    ptr += "    '#e9c46a',                                 \n";
    ptr += "    'coral',                                   \n";
    ptr += "    'rgb(231, 111, 81)',                     \n";
    ptr += "    'Crimson',                                 \n";
    ptr += "    '#023e8a',                                 \n";
    ptr += "    '#0077b6',                                 \n";
    ptr += "    'hsl(194, 100%, 39%)',                   \n";
    ptr += "    '#00b4d8',                                 \n";
    ptr += "    '#48cae4'                                  \n";
    ptr += "  ],                                           \n";
    ptr += "  onChange: (color, inputEl) => {              \n";
    ptr += "    console.log(`The new color is ${color}`);  \n";
    ptr += "  }                                            \n";
    ptr += "});                                            \n";
    ptr += "</script>n";

    return ptr;
}

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
    /* Create async web server */
    mpWebServer = new AsyncWebServer(80);
    /* and server routes */
    mpWebServer->on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(200, "text/html", Coloris());
    });

    /* Register WiFi events listener */
	WiFi.onEvent(
        std::bind(&WebSite::HandleWifiEvent, this, std::placeholders::_1));
}

void WebSite::Loop(void)
{
    // nothig to do
}

void WebSite::HandleWifiEvent(WiFiEvent_t aEvent)
{
	switch (aEvent)
	{
		case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            if (mpWebServer != nullptr)
            {
                LOG(LOG_DEBUG, "WebSite::HandleWifiEvent() Start web server");
                mpWebServer->begin();
            }
			break;

		default:
			break;
    }
}