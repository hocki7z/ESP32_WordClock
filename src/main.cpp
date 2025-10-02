#include <Arduino.h>

#include <FreeRTOScpp.h>
#include <TaskCPP.h>
#include <QueueCPP.h>
#include <TimerCPP.h>
#include <MutexCPP.h>
#include <SemaphoreCPP.h>

#include <EventCpp.h>
#include <MessageBufferCPP.h>
#include <StreamBufferCPP.h>

#include <Lock.h>
#include <Callback.h>
#include <ReadWrite.h>


#include "Display.h"


#define TEST_TASK_DEPTH_SIZE    2048
#define TEST_TASK_PRIORITY      FreeRTOScpp::TaskPrio_Low
#define TEST_TASK_NAME          "TestTask"

/* Task notification bits */
static constexpr uint32_t mNotificationMailbox = 0x01; //binary: 00000000 00000000 00000000 00000001
static constexpr uint32_t mNotificationTimer   = 0x02; //binary: 00000000 00000000 00000000 00000010


struct TaskStats
{
    char taskName[configMAX_TASK_NAME_LEN];
    UBaseType_t taskNumber;
    eTaskState taskState;
    UBaseType_t currentPriority;
    UBaseType_t basePriority;
    uint32_t runTimeCounter;
    uint32_t stackHighWaterMark;
};

typedef struct tTestTaskData
{
    uint8_t  mId;
    uint32_t mData;
} tTaskData;


typedef FreeRTOScpp::Queue<tTestTaskData, 10> tTestTaskMailBox;

class TaskNotification
{
public:
    TaskNotification(TaskHandle_t aTaskHandle, uint32_t aNotification) :
        mTaskHandle(aTaskHandle),
        mNotification(aNotification)
    {
    };

    virtual ~TaskNotification()
    {
    }

    void notify(void)
    {
        xTaskNotify(mTaskHandle, mNotification, eSetBits);
    }

    void notifyFromISR(BaseType_t* apHigherPriorityTaskWoken)
    {
        xTaskNotifyFromISR(mTaskHandle, mNotification, eSetBits, apHigherPriorityTaskWoken);
    }

private:
    TaskHandle_t    mTaskHandle;
    uint32_t        mNotification;
};

class NotificationTimer : public FreeRTOScpp::TimerClass
{
public:
    NotificationTimer(TaskHandle_t mTaskHandle, uint32_t mNotification, TickType_t aPeriod, bool aReload) :
        FreeRTOScpp::TimerClass(nullptr, aPeriod, aReload)
    {
        mpTaskNotification = new TaskNotification(mTaskHandle, mNotification);
    }

    virtual ~NotificationTimer()
    {}

    void timer(void)
    {
        if (mpTaskNotification)
        {
            mpTaskNotification->notify();
        }
    }

private:
    TaskNotification* mpTaskNotification;
};

class TestTask : public FreeRTOScpp::TaskClassS<0>
{
public:
    TestTask(tTestTaskMailBox* apMailBox) : FreeRTOScpp::TaskClassS<0>(TEST_TASK_NAME, TEST_TASK_PRIORITY, TEST_TASK_DEPTH_SIZE)
    {
        mpMailBox = apMailBox;
        this->give();
    }

    void task() override
    {
        /*
         * Task initialization
         */
        // LOG
        Serial.printf("TestTask: Run\n");

        uint32_t wResult;
        uint32_t wNotificationValue;

        tTestTaskData wData;
        uint32_t  wCount = 0;

        /*
         * Task execution code
         */
        for (;;)
        {
/*
            if (mpMailBox->pop(wData, 1000))
            {
    	        Serial.printf("TestTask: received message with id %d\n", wData.mId);
            }
            else
            {
                Serial.printf("TestTask: no messages in mailbox, count %d\n", wCount++);
            }
*/
            /* Wait to be notified */
            if (wait( /*Clear enter*/ 0, 0xFFFFFFFF, &wNotificationValue) == pdPASS)
            {
                if ((wNotificationValue & mNotificationMailbox) != 0)
                {
                    while (mpMailBox->pop(wData, 0))
                    {
                        Serial.printf("TestTask: received message with id %d\n", wData.mId);
                    }
                }

                if ((wNotificationValue & mNotificationTimer) != 0)
                {
                    Serial.printf("TestTask: received timer notification\n");
                }
            }
        }
    }

private:
    tTestTaskMailBox* mpMailBox;
};

static void HandleTimerCallback(TimerHandle_t aHandle)
{
    Serial.printf("HandleTimerCallback\n");
}

static Display*  mpDisplay;

static TestTask*    mpTestTask;
static TaskHandle_t mpTestTaskHandle;

static tTestTaskMailBox* mpTestTaskMailBox;
static TaskNotification* mpTestTaskMailBoxNotification;

static FreeRTOScpp::Timer* mpTimer;
static NotificationTimer*  mpNotificationTimer;

static FreeRTOScpp::Mutex* mpMutex;
static FreeRTOScpp::BinarySemaphore* mpSemaphore;


static uint32_t mMsgCount = 0;


void setup()
{
    /* Start the serial communication */
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    /* Create display object */
    mpDisplay = new Display();

    mpTestTaskMailBox = new tTestTaskMailBox();
    mpTestTask        = new TestTask(mpTestTaskMailBox);
    mpTestTaskHandle  = mpTestTask->getTaskHandle();

    mpTestTaskMailBoxNotification = new TaskNotification(mpTestTaskHandle, mNotificationMailbox);

//    mpTimer             = new FreeRTOScpp::Timer("TestTimer", HandleTimerCallback, 5000, true, true);
    mpNotificationTimer = new NotificationTimer(mpTestTaskHandle, mNotificationTimer, 3000, true);
    mpNotificationTimer->start();

    mpMutex     = new FreeRTOScpp::Mutex("TestMutex");
    mpSemaphore = new FreeRTOScpp::BinarySemaphore("TestSema");


    /* Initialize display */
    mpDisplay->Init();

    /* LOG */
    Serial.printf("Welcome to WordClock");
}

void loop()
{
    /* Update display */
    mpDisplay->Update();

    delay(2000);

    tTestTaskData wData;
    wData.mId   = mMsgCount++;
    wData.mData = millis();

    mpMutex->take();
    mpSemaphore->take();

    mpTestTaskMailBox->add(wData, 0);
    mpTestTaskMailBoxNotification->notify();

    mpSemaphore->give();
    mpMutex->take();
}
