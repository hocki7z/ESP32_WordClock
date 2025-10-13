#include <Arduino.h>

#include "Logger.h"
#include "Application.h"

#include "Display.h"
#include "TimeManager.h"
#include "WiFiManager.h"

/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)

static Display* mpDisplay;
static TimeManager* mpTimeManager;
static WiFiManager* mpWiFiManager;

void setup()
{
    /* Start the serial communication */
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    /* LOG */
    LOG(LOG_INFO, "Welcome to WordClock");

    /* Create tasks */
    mpDisplay     = new Display(ApplicationNS::mDisplayTaskName,
        ApplicationNS::mDisplayTaskPriority, ApplicationNS::mDisplayTaskStackSize);
    mpTimeManager = new TimeManager(ApplicationNS::mTimeManagerTaskName,
        ApplicationNS::mTimeManagerTaskPriority, ApplicationNS::mTimeManagerTaskStackSize);
    mpWiFiManager = new WiFiManager(ApplicationNS::mWifiManagerTaskName,
        ApplicationNS::mWifiManagerTaskPriority, ApplicationNS::mWifiManagerTaskStackSize);

    /* Initialize */
    mpDisplay->Init();
    mpTimeManager->Init();
    mpWiFiManager->Init();

    /* Register display as a callback for time manager */
    mpTimeManager->RegisterMinuteEventCallback(mpDisplay);
}

void loop()
{
   /* Update WiFi manager */
    mpWiFiManager->Loop();

    /* Update time manager */
    mpTimeManager->Loop();

    /* Update display */
    mpDisplay->Loop();
}
