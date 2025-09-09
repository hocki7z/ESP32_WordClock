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
