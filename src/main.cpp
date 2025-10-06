#include <Arduino.h>

#include "Display.h"

/* Check if core value is valid */
#define taskVALID_CORE_ID(xCoreID)  ((BaseType_t)((0 <= xCoreID) && (xCoreID < configNUM_CORES)))

#include <RTOScppTask.h>
#include <RTOScppQueue.h>
#include <RTOScppTimer.h>


static Display* mpDisplay;

enum tTaskPriority
{
	TaskPrio_Idle     = 0,													     ///< Non-Real Time operations. tasks that don't block
	TaskPrio_Low      = ((configMAX_PRIORITIES)>1),                         	 ///< Non-Critical operations
	TaskPrio_HMI      = (TaskPrio_Low + ((configMAX_PRIORITIES)>5)),			 ///< Normal User Interface Level
	TaskPrio_Mid      = ((configMAX_PRIORITIES)/2),							     ///< Semi-Critical, have deadlines, not a lot of processing
	TaskPrio_High     = ((configMAX_PRIORITIES)-1-((configMAX_PRIORITIES)>4)),   ///< Urgent tasks, short deadlines, not much processing
	TaskPrio_Highest  = ((configMAX_PRIORITIES)-1) 						         ///< Critical Tasks, Do NOW, must be quick (Used by FreeRTOS)
};

#define TEST_TASK_DEPTH_SIZE    (4 * 1024)
#define TEST_TASK_PRIORITY      tTaskPriority::TaskPrio_Low
#define TEST_TASK_NAME          "TestTask"

/* Task notification bits */
static constexpr uint32_t mNotificationMailbox = 0x01; //binary: 00000000 00000000 00000000 00000001
static constexpr uint32_t mNotificationTimer   = 0x02; //binary: 00000000 00000000 00000000 00000010

typedef struct tTestTaskData
{
    uint8_t  mId;
    uint32_t mData;
} tTaskData;

typedef RTOS::Queues::QueueStatic<tTestTaskData, 10> tTestTaskMailBox;


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

class NotificationTimer : public RTOS::Timers::TimerStatic
{
public:
    NotificationTimer(TaskHandle_t mTaskHandle, uint32_t mNotification, TickType_t aPeriod, bool aReload) :
        RTOS::Timers::TimerStatic("NotifyTimer", TimerCallback, pdMS_TO_TICKS(aPeriod), this, aReload, false)
    {
        mpTaskNotification = new TaskNotification(mTaskHandle, mNotification);
    };

    ~NotificationTimer()
    {
    };

private:

    TaskNotification* mpTaskNotification;

    static void TimerCallback(TimerHandle_t aTimerHandle)
    {
        /* LOG */
        Serial.printf("CustomTask::RouteTimer()\n");

        NotificationTimer* wpNotificationTimer = static_cast<NotificationTimer*>(pvTimerGetTimerID(aTimerHandle));
        if (wpNotificationTimer)
        {
            wpNotificationTimer->ExecuteTimer();
        }
    }

    void ExecuteTimer(void)
    {
        if (mpTaskNotification)
        {
            mpTaskNotification->notify();
        }
    }
};

template <uint32_t StackSize>
class TestTask : public RTOS::Tasks::TaskStatic<StackSize>
{
public:
    TestTask(const char* apName, uint8_t aPriority) :
        RTOS::Tasks::TaskStatic<StackSize>(apName, RouteTask, aPriority, this)
    {
        /* LOG */
        Serial.printf("TestTask::CustomTask()\n");

    }

    ~TestTask()
    {
    }

    void Init(tTestTaskMailBox* apMailBox)
    {
        mpMailBox = apMailBox;
    }

private:

    tTestTaskMailBox* mpMailBox;

    static void RouteTask(void* apTask)
    {
        /* LOG */
        Serial.printf("CustomTask::RouteTask()\n");

        TestTask* wpCustomTask = static_cast<TestTask*>(apTask);
        if (wpCustomTask)
        {
            wpCustomTask->ExecuteTask();
        }
    }

    void ExecuteTask(void)
    {
        uint32_t wNotificationValue;
        tTestTaskData wData;

        /*
         * Task execution code
         */
        for (;;)
        {
            if (this->notifyWait(0, 0xFFFFFFFF, wNotificationValue, portMAX_DELAY) == pdPASS)
            {
                if ((wNotificationValue & mNotificationMailbox) != 0)
                {
                    //while (mpMailBox->pop(wData, 0))
                    {
                        //Serial.printf("TestTask: received message with id %d\n", wData.mId);
                        Serial.printf("TestTask: received message\n");
                    }
                }

                if ((wNotificationValue & mNotificationTimer) != 0)
                {
                    Serial.printf("TestTask: received timer notification\n");
                }
            }
        }
    }
};


static RTOS::Tasks::ITask*      mpTestTask;
static TaskHandle_t             mpTestTaskHandle;

static tTestTaskMailBox*        mpTestTaskMailBox;
static TaskNotification*        mpTestTaskMailBoxNotification;

static RTOS::Timers::ITimer*    mpNotificationTimer;

static uint32_t mMsgCount = 0;


void setup()
{
    /* Start the serial communication */
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    /* Create display object */
    mpDisplay = new Display();

    mpTestTask = new TestTask<TEST_TASK_DEPTH_SIZE>(TEST_TASK_NAME, TEST_TASK_PRIORITY);
    mpTestTaskHandle = mpTestTask->getHandle();

    mpTestTaskMailBox = new tTestTaskMailBox();
    mpTestTaskMailBoxNotification = new TaskNotification(mpTestTask->getHandle(), mNotificationMailbox);

    static_cast<TestTask<TEST_TASK_DEPTH_SIZE>*>(mpTestTask)->Init(mpTestTaskMailBox);

    // Create task
    if (!mpTestTask->create())
    {
        Serial.printf("[ERROR] Task not created!\n");
    }

    mpNotificationTimer = new NotificationTimer(mpTestTask->getHandle(), mNotificationTimer, 3000, true);
    mpNotificationTimer->start();

    /* Initialize display */
    mpDisplay->Init();

    /* LOG */
    Serial.printf("Welcome to WordClock\n");
}

void loop()
{
    /* Update display */
    mpDisplay->Update();

    delay(2000);

    tTestTaskData wData;
    wData.mId   = mMsgCount++;
    wData.mData = millis();

    //mpTestTaskMailBox->add(wData, 0);
    //mpTestTaskMailBoxNotification->notify();
}
