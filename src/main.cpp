#include <Arduino.h>

#include <FreeRTOScpp.h>
#include <TaskCPP.h>

#include "Display.h"

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

#define TEST_TASK_DEPTH_SIZE  1014
#define TEST_TASK_PRIORITY    FreeRTOScpp::TaskPrio_Low
#define TEST_TASK_NAME         "TestTask"

class TestTask : public FreeRTOScpp::TaskClassS<0>
{
public:
    TestTask() : FreeRTOScpp::TaskClassS<0>(TEST_TASK_NAME, TEST_TASK_PRIORITY, TEST_TASK_DEPTH_SIZE)
    {
        this->give();
    }

    void task() override
    {
        TaskStats stats;

        for (;;)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));

            /* Get stats */
//            vTaskGetInfo(NULL, (TaskStatus_t *)&stats, pdTRUE, eInvalid);

            /* LOG */
            Serial.printf("TestTask: Run\n");
//            Serial.printf("Task: %s, State: %d, Prio: %d, Stack: %d\n",
//                          stats.taskName,
//                          stats.taskState,
//                          stats.currentPriority,
//                          stats.stackHighWaterMark);
        }
    }
};



static Display*  mpDisplay;

static TestTask* mpTestTask;





void setup()
{
    /* Start the serial communication */
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    /* Create display object */
    mpDisplay = new Display();

    mpTestTask = new TestTask();

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
