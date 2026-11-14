#include "Global.h"

#include "Application.h"
#include "Communication.h"
#include "Message.h"

#include "Display.h"
#include "TimeManager.h"
#include "WifiManager.h"
#include "WebSite.h"


// Diagnostic switches are shared by all translation units in TaskDiagnostics.h.
#if TASK_DIAGNOSTICS_ENABLED
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

/******************************************************************************
    Multi Reset Detector Library
 *****************************************************************************/
/* These definitions must be placed before #include <ESP_MultiResetDetector.h> to be used.
 * Otherwise, default values (MRD_TIMES = 3, MRD_TIMEOUT = 10 seconds and MRD_ADDRESS = 0)
 * will be used
 */
// Number of subsequent resets during MRD_TIMEOUT to activate
#define MRD_TIMES                   3

// Number of seconds after reset during which a subsequent reset will be considered a multi reset.
#define MRD_TIMEOUT                 10

// RTC/EEPROM Memory Address for the MultiResetDetector to use
#define MRD_ADDRESS                 0

// Enable debug output from the MultiResetDetector library
#define MULTIRESETDETECTOR_DEBUG    false

// Enable/Disable the use of LittleFS, SPIFFS or EEPROM for storing the MultiResetDetector data
#define ESP_MRD_USE_LITTLEFS        false
#define ESP_MRD_USE_SPIFFS          false
#define ESP_MRD_USE_EEPROM          true

#include <ESP_MultiResetDetector.h>


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
static void MultiResetDetection(void);
static void InitApplication(void);
static void RunApplication(void);

#if TASK_DIAGNOSTICS_ENABLED
static void LogTaskStatistics(void);
#endif


/******************************************************************************
    PRIVATE MEMBER VARIABLES
 *****************************************************************************/
/** @brief Pointer to the multi reset detector instance */
static MultiResetDetector* mpMultiResetDetector;

/** @brief Pointer to the communication manager instance */
static CommunicationNS::CommunicationManager* mpCommunicationManager = nullptr;

/** @brief Number of tasks in the system */
static constexpr size_t mcNumberOfTasks = 4;
/** @brief Array of task management structures */
static ApplicationNS::tTaskManagement mTaskManagement[mcNumberOfTasks];

#if TASK_DIAGNOSTICS_ENABLED
/** @brief Period between task and shared-heap reports, in milliseconds */
static constexpr uint32_t mcTaskStatisticsIntervalMs = 15000;
#endif


/******************************************************************************
    PUBLIC MEMBER VARIABLES
 *****************************************************************************/

/* Instance of Settings class */
SettingsNS::Settings Settings;


/******************************************************************************
    PUBLIC FUNCTION CODE
 *****************************************************************************/

 /**
  * @brief Arduino setup function
  */
void setup(void)
{
    /* Start the serial communication */
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    /* LOG */
    LOG(LOG_INFO, "Welcome to WordClock");

    /* Check multi reset */
    MultiResetDetection();

    /* Check the reason for the last system reset */
    RestartReasonNS::CheckResetReason();

    /* Initialize application */
    InitApplication();

    /* Run application */
    RunApplication();
}

/**
  * @brief Arduino loop function
  */
void loop(void)
{
#if TASK_DIAGNOSTICS_ENABLED
    static uint32_t wLastTaskStatisticsMs = millis();
    const uint32_t wNowMs = millis();

    /* Unsigned subtraction keeps the interval correct across millis() rollover. */
    if ((wNowMs - wLastTaskStatisticsMs) >= mcTaskStatisticsIntervalMs)
    {
        wLastTaskStatisticsMs = wNowMs;
        LogTaskStatistics();
    }
#endif

    /* Call the multi reset detector loop method every so often,
       so that it can recognise when the timeout expires. */
    if ((mpMultiResetDetector != nullptr) &&
        (mpMultiResetDetector->waitingForMRD()))
    {
        mpMultiResetDetector->loop();

        /* Waiting during multi reset detection */
        vTaskDelay(10);
    }
    else
    {
#if TASK_DIAGNOSTICS_ENABLED
        /* Reuse the loop task for diagnostics instead of allocating another task. */
        vTaskDelay(pdMS_TO_TICKS(100));
#else
        /* Do nothing, everything is handled in tasks */
        vTaskDelay(portMAX_DELAY);
#endif
    }
}


/******************************************************************************
    PRIVATE FUNCTION CODE
 *****************************************************************************/

#if TASK_DIAGNOSTICS_ENABLED
/**
 * @brief Log shared heap health and minimum-ever unused task stacks in bytes.
 *
 * Heap allocations are shared, not owned by individual tasks. The stock Arduino
 * framework disables trace support, so only stable application handles and the
 * current loop task can safely be queried; panic snapshot APIs are not suitable
 * for live task enumeration. Application task handles remain valid for the life
 * of this application. Stack watermarks are bytes on ESP-IDF, not FreeRTOS words.
 */
static void LogTaskStatistics(void)
{
    LOG(LOG_INFO, "Main::TaskStatistics()\theap bytes:\ttotal=%u\tfree=%u\tminFree=%u\tlargestBlock=%u\tsystemTasks=%u",
        static_cast<unsigned>(ESP.getHeapSize()),
        static_cast<unsigned>(ESP.getFreeHeap()),
        static_cast<unsigned>(ESP.getMinFreeHeap()),
        static_cast<unsigned>(ESP.getMaxAllocHeap()),
        static_cast<unsigned>(uxTaskGetNumberOfTasks()));

    /* Query only known, long-lived handles: a deleted task handle is unsafe. */
    auto wLogTask = [](TaskHandle_t aTaskHandle, uint32_t aStackBytes) -> void
    {
        if (aTaskHandle == nullptr)
        {
            return;
        }

        const char* wpState = "invalid";
        switch (eTaskGetState(aTaskHandle))
        {
            case eRunning: wpState = "running"; break;
            case eReady: wpState = "ready"; break;
            case eBlocked: wpState = "blocked"; break;
            case eSuspended: wpState = "suspended"; break;
            case eDeleted: wpState = "deleted"; break;
            default: break;
        }
        const BaseType_t wAffinity = xTaskGetAffinity(aTaskHandle);
        const char* wpAffinity = wAffinity == tskNO_AFFINITY ? "any" :
            (wAffinity == 0 ? "CPU0" : (wAffinity == 1 ? "CPU1" : "unknown"));
        const uint32_t wFree = uxTaskGetStackHighWaterMark(aTaskHandle);
        // Avoid unsigned underflow if allocation metadata does not match the watermark.
        const bool wValidStack = aStackBytes != 0 && wFree <= aStackBytes;
        const uint32_t wUsed = wValidStack ? aStackBytes - wFree : 0;
        const unsigned wPercent = wValidStack ?
            static_cast<unsigned>((static_cast<uint64_t>(wUsed) * 100) / aStackBytes) : 0;

        LOG(LOG_INFO, "Main::TaskStatistics()\t%-16s  %4u  %-9s  %-8s  %7u  %8u  %9u  %7u%%%s",
            pcTaskGetName(aTaskHandle),
            static_cast<unsigned>(uxTaskPriorityGet(aTaskHandle)),
            wpState, wpAffinity, static_cast<unsigned>(aStackBytes),
            static_cast<unsigned>(wFree), static_cast<unsigned>(wUsed), wPercent,
            wValidStack ? "" : "  invalid stack metadata");
    };

    LOG(LOG_INFO, "Main::TaskStatistics()\tstack scope: application tasks + Arduino loop (not all system tasks)");
    // Matching header/data widths avoid dependence on terminal tab-stop settings.
    LOG(LOG_INFO, "Main::TaskStatistics()\t%-16s  %4s  %-9s  %-8s  %7s  %8s  %9s  %8s",
        "Task", "Prio", "State", "Affinity", "StackB", "MinFreeB", "PeakUsedB", "PeakUsed");
    for (const auto& wrTaskManagement : mTaskManagement)
    {
        if (wrTaskManagement.mpTask != nullptr)
        {
            wLogTask(wrTaskManagement.mpTask->getTaskHandle(),
                wrTaskManagement.mpTask->GetConfiguredStackSize());
        }
    }
    wLogTask(xTaskGetCurrentTaskHandle(), getArduinoLoopTaskStackSize());

    LOG(LOG_INFO, "Main::TaskStatistics()\tqueues (application messages; peak is sampled)");
    LOG(LOG_INFO, "Main::TaskStatistics()\t%-16s  %5s  %8s  %12s  %11s",
        "Task", "Depth", "Capacity", "ObservedPeak", "FailedSends");
    for (const auto& wrTaskManagement : mTaskManagement)
    {
        if (wrTaskManagement.mpTask == nullptr || wrTaskManagement.mpMessageQueue == nullptr)
        {
            continue;
        }
        const char* wpName = pcTaskGetName(wrTaskManagement.mpTask->getTaskHandle());
        const auto wQueue = wrTaskManagement.mpMessageQueue->GetStatistics();
        LOG(LOG_INFO, "Main::TaskStatistics()\t%-16s  %5u  %8u  %12u  %11u",
            wpName, static_cast<unsigned>(wQueue.CurrentDepth), static_cast<unsigned>(wQueue.Capacity),
            static_cast<unsigned>(wQueue.PeakDepth), static_cast<unsigned>(wQueue.FailedSends));
    }

    LOG(LOG_INFO, "Main::TaskStatistics()\ttiming (elapsed wall time, not CPU time)");
    LOG(LOG_INFO, "Main::TaskStatistics()\t%-16s  %-18s  %10s  %10s  %10s",
        "Task", "Metric", "Count", "LastUs", "MaxUs");
    for (const auto& wrTaskManagement : mTaskManagement)
    {
        if (wrTaskManagement.mpTask == nullptr)
        {
            continue;
        }
        const char* wpName = pcTaskGetName(wrTaskManagement.mpTask->getTaskHandle());
        const auto wStats = wrTaskManagement.mpTask->GetStatistics();
        // Measurements are elapsed wall time, including preemption, not CPU utilization.
        auto wLogTiming = [wpName](const char* apMetric, const ApplicationNS::TimingStatistics& arTiming)
        {
            LOG(LOG_INFO, "Main::TaskStatistics()\t%-16s  %-18s  %10u  %10u  %10u",
                wpName, apMetric, static_cast<unsigned>(arTiming.Count),
                static_cast<unsigned>(arTiming.LastUs), static_cast<unsigned>(arTiming.MaxUs));
        };
        wLogTiming("messageHandler", wStats.IncomingMessage);
        wLogTiming("timerHandler", wStats.TimerHandler);
        wLogTiming("notificationBatch", wStats.UnknownNotification);
        wLogTiming("timerDeliveryDelay", wStats.TimerDeliveryDelayUs);
    }
}
#endif

/**
 * @brief Detect a multi reset condition
 *
 * @details
 * This function creates an instance of the MultiResetDetector class and checks
 * if a multi reset condition has been detected. If a multi reset is detected,
 * it clears the WiFi settings to force the device to enter WiFi AP mode.
 */
static void MultiResetDetection(void)
{
    /* Create instance of MultiResetDetector */
    mpMultiResetDetector = new MultiResetDetector(MRD_TIMEOUT, MRD_ADDRESS);

    /* Check if a multi reset has been detected */
    if (mpMultiResetDetector->detectMultiReset())
    {
        /* LOG */
        LOG(LOG_DEBUG, "Main::MultiResetDetection() Multi Reset Detected");

        /* Stop detection */
        mpMultiResetDetector->stop();

        /* Clear WiFi settings to force the device to enter WIFI AP mode */
        Settings.RemoveKey(ConfigNS::mKeyWifiSSID);
        Settings.RemoveKey(ConfigNS::mKeyWifiPassword);
    }
}

/**
 * @brief Initialize the application
 *
 * @details
 * This function initializes the application by synchronizing the flash partition with the firmware,
 * creating the communication manager, and creating all necessary tasks. It uses a lambda function
 * to create tasks based on their addresses and initializes their message queues and receivers.
 */
static void InitApplication(void)
{
    /* Create communication manager */
    mpCommunicationManager = new CommunicationNS::CommunicationManager();

    /* Lambda function to create a task based on the given address */
    auto wCreateTask = [](CommunicationNS::CommunicationManager* apCommunicationManager, tTaskManagement& arTaskManagement,
        MessageNS::tAddress mAddress, const char* apTaskName, ApplicationNS::tTaskPriority aTaskPriority, uint32_t aTaskStackSize) -> bool
    {
        /* Flag indicating whether the task was created successfully */
        bool wTaskCreated = false;  // default - not created

        /* Check given address */
        switch (mAddress)
        {
            case MessageNS::tAddress::DISPLAY_MANAGER:
                arTaskManagement.mpTask = new Display(apTaskName, aTaskPriority, aTaskStackSize);
                wTaskCreated = true;
                break;

            case MessageNS::tAddress::TIME_MANAGER:
                arTaskManagement.mpTask = new TimeManager(apTaskName, aTaskPriority, aTaskStackSize);
                wTaskCreated = true;
                break;

            case MessageNS::tAddress::WIFI_MANAGER:
                arTaskManagement.mpTask = new WiFiManager(apTaskName, aTaskPriority, aTaskStackSize);
                wTaskCreated = true;
                break;

            case MessageNS::tAddress::WEB_MANAGER:
                arTaskManagement.mpTask = new WebSite(apTaskName, aTaskPriority, aTaskStackSize);
                wTaskCreated = true;
                break;

            default:
                // Unsupported address, nothing to do here
                break;
        }

        /* If the task was created successfully */
        if (wTaskCreated)
        {
            LOG(LOG_DEBUG, "Main::CreateTask() Created task %s, priority %d, stack size %d",
                    apTaskName, aTaskPriority, aTaskStackSize);

            /* Set the task address in the task management structure */
            arTaskManagement.mAddress = mAddress;

            /* Create message queue and receiver */
            arTaskManagement.mpMessageQueue    = new ApplicationNS::MessageQueue();
            arTaskManagement.mpMessageReceiver = new ApplicationNS::MessageReceiver();

            /* Initialize task objects */
            arTaskManagement.mTaskObjects.mpMessageQueue = arTaskManagement.mpMessageQueue;
            arTaskManagement.mTaskObjects.mpCommunicationManager = apCommunicationManager;

            /* Initialize task */
            arTaskManagement.mpTask->Init(&arTaskManagement.mTaskObjects);

            /* Initialize and register messages receiver */
            arTaskManagement.mpMessageReceiver->Init(arTaskManagement.mpMessageQueue,
                arTaskManagement.mpTask->getTaskHandle(), ApplicationNS::mTaskNotificationMsgQueue);

            apCommunicationManager->RegisterCallback(
                mAddress, arTaskManagement.mpMessageReceiver);
        }
        else
        {
            LOG(LOG_ERROR, "Main::CreateTask() Failed to create task %s, unsupported address %d",
                    apTaskName, mAddress);
        }

        return wTaskCreated;    // Return whether the task was created or not
    };

    /* Create all tasks */
    uint8_t wTaskNumber = 0;

    wCreateTask(mpCommunicationManager, mTaskManagement[wTaskNumber++], MessageNS::tAddress::DISPLAY_MANAGER,
        ConfigNS::mDisplayTaskName, ConfigNS::mDisplayTaskPriority, ConfigNS::mDisplayTaskStackSize);

    wCreateTask(mpCommunicationManager, mTaskManagement[wTaskNumber++], MessageNS::tAddress::TIME_MANAGER,
        ConfigNS::mTimeManagerTaskName, ConfigNS::mTimeManagerTaskPriority, ConfigNS::mTimeManagerTaskStackSize);

    wCreateTask(mpCommunicationManager, mTaskManagement[wTaskNumber++], MessageNS::tAddress::WIFI_MANAGER,
        ConfigNS::mWifiManagerTaskName, ConfigNS::mWifiManagerTaskPriority, ConfigNS::mWifiManagerTaskStackSize);

    wCreateTask(mpCommunicationManager, mTaskManagement[wTaskNumber++], MessageNS::tAddress::WEB_MANAGER,
        ConfigNS::mWebSiteTaskName, ConfigNS::mWebSiteTaskPriority, ConfigNS::mWebSiteTaskStackSize);
}

/**
 * @brief Run the application
 *
 * @details
 * This function triggers all created tasks by notifying them.
 */
static void RunApplication(void)
{
    /* Lambda function to run a task based on the given task management structure */
    auto wRunTask = [](ApplicationNS::tTaskManagement& arTaskManagement) -> void
    {
        /* Check if the task is created and has a valid handle */
        if ((arTaskManagement.mpTask) &&
            (arTaskManagement.mpTask->getTaskHandle()))
        {
            /* Trigger task by notifying it */
            xTaskNotifyGive(arTaskManagement.mpTask->getTaskHandle());
        }
        else
        {
            LOG(LOG_ERROR, "Main::RunTask() Task not created or task handle is null for address %d",
                arTaskManagement.mAddress);
        }
    };

    /* Trigger all tasks */
    for (uint8_t wI=0; wI < mcNumberOfTasks; wI++)
    {
        wRunTask(mTaskManagement[wI]);
    }
}