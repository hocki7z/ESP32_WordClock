#include <Arduino.h>
#include <esp_task_wdt.h>

#include "Logger.h"
#include "Display.h"
#include "TimeManager.h"
#include "WiFiManager.h"
#include "WebSite.h"

/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)


static Display* mpDisplay;
static TimeManager* mpTimeManager;
static WiFiManager* mpWiFiManager;
static WebSite* mpWebSite;

void setup()
{
    /* Start the serial communication */
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    /* LOG */
    LOG(LOG_INFO, "Welcome to WordClock");

    // Task Watchdog configuration
    esp_task_wdt_deinit();
    esp_err_t ESP32_ERROR = esp_task_wdt_init(/*Timeout in seconds*/ 25, /*Enable panic to restart ESP32*/ true);
    LOG(LOG_DEBUG, "Main::Setup() Last Reset %s", String(esp_err_to_name(ESP32_ERROR)));
    esp_task_wdt_add(NULL);  //add current thread to WDT watch


    /* Create objects */
    mpDisplay       = new Display();
    mpTimeManager   = new TimeManager();
    mpWiFiManager   = new WiFiManager();
    mpWebSite       = new WebSite();

    /* Initialize */
    mpDisplay->Init();
    mpTimeManager->Init();
    mpWiFiManager->Init();
    mpWebSite->Init();

    /* Register display as a callback for time manager */
    mpTimeManager->RegisterMinuteEventCallback(mpDisplay);
}

void loop()
{
    esp_task_wdt_reset();

   /* Update WiFi manager */
    mpWiFiManager->Loop();

    /* Update time manager */
    mpTimeManager->Loop();

    /* Update display */
    mpDisplay->Loop();

    /* Update web site */
    mpWebSite->Loop();
}
