#include <Arduino.h>

#include "Logger.h"
#include "Settings.hpp"

#include "Configuration.h"

#include "Application.h"
#include "Communication.h"
#include "Message.h"

#include "Display.h"
#include "TimeManager.h"
#include "WiFiManager.h"


/******************************************************************************
    PRIVATE SYMBOLIC CONSTANTS AND MACROS
 *****************************************************************************/
/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)


/******************************************************************************
    PRIVATE TYPES, STRUCTURES, UNIONS AND ENUMS
 *****************************************************************************/


/******************************************************************************
    PRIVATE FUNCTION PROTOTYPES
 *****************************************************************************/
static void CheckResetReason(void);
static void InitApplication(void);
static void RunApplication(void);


/******************************************************************************
    PRIVATE MEMBER VARIABLES
 *****************************************************************************/
static Display* mpDisplay;
static TimeManager* mpTimeManager;
static WiFiManager* mpWiFiManager;

static CommunicationNS::CommunicationManager* mpCommunicationManager = nullptr;

static ApplicationNS::MessageQueue*     mpDisplayMessageQueue;
static ApplicationNS::MessageReceiver*  mpDisplayMessageReceiver;
static ApplicationNS::tTaskObjects      mDisplayTaskObjects;

static ApplicationNS::MessageQueue*     mpTimeManagerMessageQueue;
static ApplicationNS::MessageReceiver*  mpTimeManagerMessageReceiver;
static ApplicationNS::tTaskObjects      mTimeManagerTaskObjects;

static ApplicationNS::MessageQueue*     mpWifiManagerMessageQueue;
static ApplicationNS::MessageReceiver*  mpWifiManagerMessageReceiver;
static ApplicationNS::tTaskObjects      mWifiManagerTaskObjects;


/******************************************************************************
    PUBLIC MEMBER VARIABLES
 *****************************************************************************/

/* Instance of Settings class */
SettingsNS::Settings Settings;


/******************************************************************************
    PUBLIC FUNCTION CODE
 *****************************************************************************/
void setup()
{
    /* Start the serial communication */
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    /* LOG */
    LOG(LOG_INFO, "Welcome to WordClock");

    /* Check the reason for the last system reset */
    CheckResetReason();

    /* Initialize application */
    InitApplication();

    /* Run application */
    RunApplication();
}

void loop()
{
    /* Do nothing, everything is handled in tasks */
    vTaskDelay(portMAX_DELAY);
}


/******************************************************************************
    PRIVATE FUNCTION CODE
 *****************************************************************************/
static void CheckResetReason(void)
{
    /* Determine the reason for the last system reset */
    esp_reset_reason_t wReason = esp_reset_reason();

    /* Update corresponding counters in settings */
    switch (wReason)
    {
        case ESP_RST_POWERON:   // Power-on reset
            Settings.IncreaseCounter(ConfigNS::mKeyCounterResetPowerOn);
            LOG(LOG_DEBUG, "Main::CheckResetReason() Power-on reset, %d times",
                    Settings.GetCounter(ConfigNS::mKeyCounterResetPowerOn));
            break;

        case ESP_RST_SW:        // Software reset
            Settings.IncreaseCounter(ConfigNS::mKeyCounterResetSoftware);
            LOG(LOG_DEBUG, "Main::CheckResetReason() Software reset, %d times",
                    Settings.GetCounter(ConfigNS::mKeyCounterResetSoftware));
            break;

        case ESP_RST_WDT:       // Watchdog reset
        case ESP_RST_INT_WDT:   // Interrupt watchdog reset
        case ESP_RST_TASK_WDT:  // Task watchdog reset
            Settings.IncreaseCounter(ConfigNS::mKeyCounterResetWatchdog);
            LOG(LOG_DEBUG, "Main::CheckResetReason() Watchdog reset, %d times",
                    Settings.GetCounter(ConfigNS::mKeyCounterResetWatchdog));
            break;

        case ESP_RST_PANIC:     // Panic reset
            Settings.IncreaseCounter(ConfigNS::mKeyCounterResetPanic);
            LOG(LOG_DEBUG, "Main::CheckResetReason() Panic reset, %d times",
                    Settings.GetCounter(ConfigNS::mKeyCounterResetPanic));
            break;

        case ESP_RST_BROWNOUT:  // Brownout reset
            Settings.IncreaseCounter(ConfigNS::mKeyCounterResetBrownout);
            LOG(LOG_DEBUG, "Main::CheckResetReason() Brownout reset, %d times",
                    Settings.GetCounter(ConfigNS::mKeyCounterResetBrownout));
            break;

        case ESP_RST_EXT:       // External reset
        case ESP_RST_DEEPSLEEP: // Deep sleep reset
        case ESP_RST_SDIO:      // SDIO reset
        case ESP_RST_UNKNOWN:   // unknown reset reason
        default:
            LOG(LOG_DEBUG, "Main::CheckResetReason() Other reset reason: %d", wReason);
            // other reset reasons are not counted
            break;
    }
}

static void InitApplication(void)
{
    /* Create tasks */
    mpDisplay = new Display(ConfigNS::mDisplayTaskName, ConfigNS::mDisplayTaskPriority,
            ConfigNS::mDisplayTaskStackSize);
    mpTimeManager = new TimeManager(ConfigNS::mTimeManagerTaskName, ConfigNS::mTimeManagerTaskPriority,
            ConfigNS::mTimeManagerTaskStackSize);
    mpWiFiManager = new WiFiManager(ConfigNS::mWifiManagerTaskName, ConfigNS::mWifiManagerTaskPriority,
            ConfigNS::mWifiManagerTaskStackSize);

    /* Create communication manager */
    mpCommunicationManager = new CommunicationNS::CommunicationManager();

    /* Create message queue and receiver for tasks */
    mpDisplayMessageQueue        = new ApplicationNS::MessageQueue();
    mpDisplayMessageReceiver     = new ApplicationNS::MessageReceiver();

    mpTimeManagerMessageQueue    = new ApplicationNS::MessageQueue();
    mpTimeManagerMessageReceiver = new ApplicationNS::MessageReceiver();

    mpWifiManagerMessageQueue    = new ApplicationNS::MessageQueue();
    mpWifiManagerMessageReceiver = new ApplicationNS::MessageReceiver();

    /* Initialize task objects */
    mpDisplayMessageReceiver->Init(mpDisplayMessageQueue,
        mpDisplay->getTaskHandle(), ApplicationNS::mTaskNotificationMsgQueue);
    mpTimeManagerMessageReceiver->Init(mpTimeManagerMessageQueue,
        mpTimeManager->getTaskHandle(), ApplicationNS::mTaskNotificationMsgQueue);
    mpWifiManagerMessageReceiver->Init(mpWifiManagerMessageQueue,
        mpWiFiManager->getTaskHandle(), ApplicationNS::mTaskNotificationMsgQueue);

    mDisplayTaskObjects.mpMessageQueue             = mpDisplayMessageQueue;
    mDisplayTaskObjects.mpCommunicationManager     = mpCommunicationManager;

    mTimeManagerTaskObjects.mpMessageQueue         = mpTimeManagerMessageQueue;
    mTimeManagerTaskObjects.mpCommunicationManager = mpCommunicationManager;

    mWifiManagerTaskObjects.mpMessageQueue         = mpWifiManagerMessageQueue;
    mWifiManagerTaskObjects.mpCommunicationManager = mpCommunicationManager;

    /* Initialize tasks */
    mpDisplay->Init(&mDisplayTaskObjects);
    mpTimeManager->Init(&mTimeManagerTaskObjects);
    mpWiFiManager->Init(&mWifiManagerTaskObjects);

    /* Register messages receiver an communication manager */
    mpCommunicationManager->RegisterCallback(
        MessageNS::tAddress::DISPLAY_MANAGER, mpDisplayMessageReceiver);
    mpCommunicationManager->RegisterCallback(
        MessageNS::tAddress::TIME_MANAGER,    mpTimeManagerMessageReceiver);
    mpCommunicationManager->RegisterCallback(
        MessageNS::tAddress::WIFI_MANAGER,    mpWifiManagerMessageReceiver);
}

static void RunApplication(void)
{
    /* Trigger all tasks */
    if ((mpDisplay != nullptr) &&
        (mpDisplay->getTaskHandle() != nullptr))
    {
            xTaskNotifyGive(mpDisplay->getTaskHandle());
    }

    if ((mpTimeManager != nullptr) &&
        (mpTimeManager->getTaskHandle() != nullptr))
    {
            xTaskNotifyGive(mpTimeManager->getTaskHandle());
    }

    if ((mpWiFiManager != nullptr) &&
        (mpWiFiManager->getTaskHandle() != nullptr))
    {
            xTaskNotifyGive(mpWiFiManager->getTaskHandle());
    }
}