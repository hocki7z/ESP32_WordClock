#include <Arduino.h>

#include "Display.h"


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
    Serial.printf("Welcome to WordClock");
}

void loop()
{
    /* Update display */
    mpDisplay->Update();
}
