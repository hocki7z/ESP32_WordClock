#include <Arduino.h>
#include <esp_task_wdt.h>

#include "Logger.h"
#include "Display.h"
#include "TimeManager.h"
#include "WiFiManager.h"

/* Log level for this module */
#define LOG_LEVEL       (LOG_DEBUG)

/* WDT Timeout in seconds */
#define WDT_TIMEOUT     (15)


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

    /*  Initialize ESP32 task watchdog */
    esp_task_wdt_deinit();

    //esp_err_t wEspError = esp_task_wdt_init(WDT_TIMEOUT, true);

    esp_task_wdt_config_t wWdtConfig = {
        .timeout_ms = WDT_TIMEOUT * 1000,                 // Convertin ms
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,  // Bitmask of all cores, https://github.com/espressif/esp-idf/blob/v5.2.2/examples/system/task_watchdog/main/task_watchdog_example_main.c
        .trigger_panic = true                             // Enable panic to restart ESP32
    };
    esp_err_t wEspError = esp_task_wdt_init(&wWdtConfig);

    LOG(LOG_DEBUG, "Main::Setup() Last Reset %s", String(esp_err_to_name(wEspError)));
    /* add current thread to WDT watch */
    esp_task_wdt_add(NULL);


    /* Create objects */
    mpDisplay       = new Display();
    mpTimeManager   = new TimeManager();
    mpWiFiManager   = new WiFiManager();

    /* Initialize */
    mpDisplay->Init();
    mpTimeManager->Init();
    mpWiFiManager->Init();

    /* Register display as a callback for time manager */
    mpTimeManager->RegisterMinuteEventCallback(mpDisplay);
}

void loop()
{
    /* Kick the watchdog */
    esp_task_wdt_reset();

    // Test WDG
    uint8_t wTestWDTTicks = 0;
    while (1)
    {
        delay(1000);
        LOG(LOG_DEBUG, "Main::Loop() WDT Test, tick %d", wTestWDTTicks++);
    };

   /* Update WiFi manager */
    mpWiFiManager->Loop();

    /* Update time manager */
    mpTimeManager->Loop();

    /* Update display */
    mpDisplay->Loop();
}
