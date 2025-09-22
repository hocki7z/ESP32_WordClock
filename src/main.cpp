#include <Arduino.h>

#include "Logger.h"
#include "Display.h"
#include "TimeManager.h"

/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)


static Display* mpDisplay;
static TimeManager* pmTimeManager;

void setup()
{
    /* Start the serial communication */
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    /* Create objects */
    mpDisplay       = new Display();
    pmTimeManager   = new TimeManager();

    /* Initialize */
    mpDisplay->Init();
    pmTimeManager->Init();

    /* Register display as a callback for time manager */
    pmTimeManager->RegisterMinuteEventCallback(mpDisplay);

    /* LOG */
    LOG(LOG_INFO, "Welcome to WordClock");
}

void loop()
{
    /* Update time manager */
    pmTimeManager->Loop();

    /* Update display */
    mpDisplay->Loop();
}
