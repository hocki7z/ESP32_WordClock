#include <Arduino.h>

#include "Display.h"
#include "TimeManager.h"


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
    Serial.printf("Welcome to WordClock");
}

void loop()
{
    /* Update time manager */
    pmTimeManager->Loop();

    /* Update display */
    mpDisplay->Loop();
}
