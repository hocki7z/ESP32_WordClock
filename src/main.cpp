#include <Arduino.h>

void setup()
{
    /* Start the serial communication */
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    /* LOG */
    Serial.printf("Welcome to WordClock");
}

void loop()
{
  // put your main code here, to run repeatedly:
}
