#include <Arduino.h>
#include <esp_system.h>

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


#define TEST_TASK_DEPTH_SIZE    2 * 1024
#define TEST_TASK_PRIORITY      FreeRTOScpp::TaskPrio_Low
#define TEST_TASK_NAME          "TestTask"

#define MESSAGE_PAYLOAD_MAX_SIZE    32
#define MESSAGE_BUFFER_SIZE         10 * ( 6 + MESSAGE_PAYLOAD_MAX_SIZE ) // (...) - mId + mTimestamp + mPayloadLen + Payload size

/* Task notification bits */
static constexpr uint32_t mNotificationTimer    = 0x01; //binary: 00000000 00000000 00000000 00000001
static constexpr uint32_t mNotificationMsgQueue = 0x02; //binary: 00000000 00000000 00000000 00000010
static constexpr uint32_t mNotificationMsgBuff  = 0x04; //binary: 00000000 00000000 00000000 00000100

typedef struct tTestTaskData
{
    uint8_t  mId = 0;

    uint32_t mTimestamp = 0;
} tTestTaskData;


class Message
{
public:
    uint8_t  mId = 0;

    uint32_t mTimestamp = 0;

    /** @brief Length of the payload */
    uint8_t  mPayloadLen = 0;
    /** @brief Pointer to the payload buffer */
    uint8_t* mpPayloadBuffer = nullptr;

    size_t GetMessageLen(void) const { return ( sizeof(mId) + sizeof(mTimestamp) + sizeof(mPayloadLen) + mPayloadLen ); };

    static uint8_t Serialize(const Message* apMessage, uint8_t* apBuffer, const size_t aBufferLen)
    {
        uint32_t wOffset = 0;

        if (aBufferLen < apMessage->GetMessageLen())
        {
            return 1; //buffer too small
        }

        /* Copy data */
        apBuffer[wOffset++] = apMessage->mId;

        apBuffer[wOffset++] = (apMessage->mTimestamp >> 24) & 0xFF;
        apBuffer[wOffset++] = (apMessage->mTimestamp >> 16) & 0xFF;
        apBuffer[wOffset++] = (apMessage->mTimestamp >>  8) & 0xFF;
        apBuffer[wOffset++] = (apMessage->mTimestamp >>  0) & 0xFF;

        if (apMessage->mPayloadLen > 0 && apMessage->mpPayloadBuffer)
        {
            apBuffer[wOffset++] = apMessage->mPayloadLen;

            for (uint8_t wI = 0; wI < apMessage->mPayloadLen; wI++)
            {
                apBuffer[wOffset++] = apMessage->mpPayloadBuffer[wI];
            }
        }
        else
        {
            apBuffer[wOffset++] = 0; //no payload
        }

        return 0; //no errors
    }

    static uint8_t Deserialize(Message* apMessage, const uint8_t* apBuffer, size_t aLength)
    {
        uint32_t wOffset = 0;

        if (aLength > apMessage->GetMessageLen())
        {
            return 1; //message length too small
        }

        /* Copy data */
        apMessage->mId = apBuffer[wOffset++];

        apMessage->mTimestamp  = ((uint32_t)apBuffer[wOffset++]) << 24;
        apMessage->mTimestamp |= ((uint32_t)apBuffer[wOffset++]) << 16;
        apMessage->mTimestamp |= ((uint32_t)apBuffer[wOffset++]) <<  8;
        apMessage->mTimestamp |= ((uint32_t)apBuffer[wOffset++]) <<  0;

        uint8_t wPayloadLen = apBuffer[wOffset++];

        if (wPayloadLen > 0 && apMessage->mpPayloadBuffer)
        {
            apMessage->mPayloadLen = wPayloadLen;

            for (uint8_t wI = 0; wI < wPayloadLen; wI++)
            {
                apMessage->mpPayloadBuffer[wI] = apBuffer[wOffset++];
            }
        }
        else
        {
            apMessage->mPayloadLen = 0;
        }

        return 0; //no errors
    }
};

typedef FreeRTOScpp::Queue<tTestTaskData, 10> tTestTaskMsgQueue;


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

class TestTaskMessageBuffer
{
public:
    TestTaskMessageBuffer()
    {
        mpMessageBuffer = new FreeRTOScpp::MessageBuffer<MESSAGE_BUFFER_SIZE>();
    }

    virtual ~TestTaskMessageBuffer()
    {
    }

    bool IsEmpty(void) const
    {
        bool wResult = true;

        if (mpMessageBuffer)
        {
            wResult = mpMessageBuffer->isEmpty();
        }

        return wResult;
    }

    size_t Send(const Message* apMessage, TickType_t aDelay = portMAX_DELAY)
    {
        size_t wSentSize = 0;

        const size_t wMessageLen = apMessage->GetMessageLen();
        uint8_t wMessageBuffer[wMessageLen];

        Message::Serialize(apMessage, wMessageBuffer, wMessageLen);

        if (mpMessageBuffer)
        {
            wSentSize = mpMessageBuffer->send(&wMessageBuffer, wMessageLen, aDelay);
        }

        return wSentSize;
    }

    uint8_t Receive(Message* apMessage, TickType_t aDelay = portMAX_DELAY)
    {
        uint8_t wResult = 1; //error by default

        const size_t wMessageLen = apMessage->GetMessageLen();
        uint8_t wMessageBuffer[wMessageLen];

        if (mpMessageBuffer)
        {
            size_t wReceivedSize = mpMessageBuffer->read(&wMessageBuffer, wMessageLen, aDelay);
            if (wReceivedSize <= wMessageLen)
            {
                wResult = Message::Deserialize(apMessage, wMessageBuffer, wReceivedSize);
            }
        }

        return wResult;
    }

private:
    FreeRTOScpp::MessageBuffer<MESSAGE_BUFFER_SIZE>* mpMessageBuffer;

};


class TestTask : public FreeRTOScpp::TaskClassS<0>
{
public:
    TestTask() : FreeRTOScpp::TaskClassS<0>(TEST_TASK_NAME, TEST_TASK_PRIORITY, TEST_TASK_DEPTH_SIZE)
    {
        this->give();
    }

    void Init(tTestTaskMsgQueue* apMsgQueue, TestTaskMessageBuffer* apMsgBuffer)
    {
        Serial.printf("TestTask: Init\n");

        mpMsgQueue  = apMsgQueue;
        mpMsgBuffer = apMsgBuffer;
    }

    void task() override
    {
        // LOG
        Serial.printf("TestTask: Run\n");

        uint32_t wResult;
        uint32_t wNotificationValue;

        uint8_t wPayload[MESSAGE_PAYLOAD_MAX_SIZE];

        /*
         * Task execution code
         */
        for (;;)
        {
            /* Wait to be notified */
            if (wait( /*Clear enter*/ 0, 0xFFFFFFFF, &wNotificationValue) == pdPASS)
            {
                if ((wNotificationValue & mNotificationTimer) != 0)
                {
                    Serial.printf("TestTask: received timer notification\n");
                }

                if ((wNotificationValue & mNotificationMsgQueue) != 0)
                {
                    tTestTaskData wData;

                    while (mpMsgQueue->pop(wData, 0))
                    {
                        Serial.printf("TestTask: received queue message with id %d, timestamp %d\n",
                            wData.mId, wData.mTimestamp);
                    }
                }

                if ((wNotificationValue & mNotificationMsgBuff) != 0)
                {
                    while (mpMsgBuffer->IsEmpty() == false)
                    {
                        Message wMessage;
                        wMessage.mPayloadLen = MESSAGE_PAYLOAD_MAX_SIZE;
                        wMessage.mpPayloadBuffer = wPayload;

                        mpMsgBuffer->Receive(&wMessage, 0);

                        Serial.printf("TestTask: received buffer message with id %d, timestamp %d, payload size %d\n",
                            wMessage.mId, wMessage.mTimestamp, wMessage.mPayloadLen);

                        if (wMessage.mpPayloadBuffer)
                        {
                            wMessage.mpPayloadBuffer = nullptr;
                        }
                    }
                }
            }
        }
    }

private:
    tTestTaskMsgQueue*      mpMsgQueue = nullptr;
    TestTaskMessageBuffer* mpMsgBuffer = nullptr;
};


static Display*  mpDisplay;

static TestTask*    mpTestTask;
static TaskHandle_t mpTestTaskHandle;

static tTestTaskMsgQueue*       mpTestTaskMsgQueue;
static TaskNotification*        mpTestTaskMsgQueueNotification;

static TestTaskMessageBuffer*   mpTestTaskMsgBuffer;
static TaskNotification*        mpTestTaskMsgBufferNotification;

static NotificationTimer*       mpNotificationTimer;


void setup()
{
    /* Start the serial communication */
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    /* Create display object */
    mpDisplay = new Display();

    mpTestTask         = new TestTask();
    mpTestTaskHandle   = mpTestTask->getTaskHandle();

    mpTestTaskMsgQueue = new tTestTaskMsgQueue();
    mpTestTaskMsgQueueNotification = new TaskNotification(mpTestTaskHandle, mNotificationMsgQueue);

    mpTestTaskMsgBuffer = new TestTaskMessageBuffer();
    mpTestTaskMsgBufferNotification = new TaskNotification(mpTestTaskHandle, mNotificationMsgBuff);

    mpTestTask->Init(mpTestTaskMsgQueue, mpTestTaskMsgBuffer);

    mpNotificationTimer = new NotificationTimer(mpTestTaskHandle, mNotificationTimer, 5000, true);
    mpNotificationTimer->start();

    /* Initialize display */
    mpDisplay->Init();

    /* LOG */
    Serial.printf("Welcome to WordClock\n");
}

void loop()
{
    static uint32_t wMsgCount = 0;

    /* Update display */
    mpDisplay->Update();


    delay(1000);

    tTestTaskData wData;
    wData.mId   = wMsgCount++;
    wData.mTimestamp = millis();

    mpTestTaskMsgQueue->add(wData, 0);
    mpTestTaskMsgQueueNotification->notify();


    delay(1000);

    Message wMessage;
    wMessage.mId = wMsgCount++;
    wMessage.mTimestamp = millis();

    // Fill payload with some random data
    uint8_t wPayloadLen = random(1, MESSAGE_PAYLOAD_MAX_SIZE-1);
    uint8_t wPayload[wPayloadLen];

    for (uint8_t wI = 0; wI < wPayloadLen; wI++)
    {
        wPayload[wI] = random(0, 0xFF);
    }

    wMessage.mPayloadLen = wPayloadLen;
    wMessage.mpPayloadBuffer = wPayload;

    mpTestTaskMsgBuffer->Send(&wMessage, 0);
    mpTestTaskMsgBufferNotification->notify();
}
