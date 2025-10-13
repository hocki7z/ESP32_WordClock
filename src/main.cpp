#include <Arduino.h>

#include "Logger.h"

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
static void InitApplication(void);


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

    /* Initialize application */
    InitApplication();
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


/******************************************************************************
    PRIVATE FUNCTION CODE
 *****************************************************************************/
static void InitApplication(void)
{
    /* Create tasks */
    mpDisplay     = new Display(ApplicationNS::mDisplayTaskName,
        ApplicationNS::mDisplayTaskPriority, ApplicationNS::mDisplayTaskStackSize);
    mpTimeManager = new TimeManager(ApplicationNS::mTimeManagerTaskName,
        ApplicationNS::mTimeManagerTaskPriority, ApplicationNS::mTimeManagerTaskStackSize);
    mpWiFiManager = new WiFiManager(ApplicationNS::mWifiManagerTaskName,
        ApplicationNS::mWifiManagerTaskPriority, ApplicationNS::mWifiManagerTaskStackSize);

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