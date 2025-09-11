#include <Arduino.h>

#include "Logger.h"
#include "Display.h"

/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)


static Display* mpDisplay;


void setup()
{
    /* Start the serial communication */
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    /* Create display object */
    mpDisplay = new Display();

    /* Initialize display */
    mpDisplay->Init();

    /* LOG */
    LOG(LOG_INFO, "Welcome to WordClock");
}

void loop()
{
    /* Update display */
    mpDisplay->Update();
}
