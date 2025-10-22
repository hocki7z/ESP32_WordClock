/*
 * Application.cpp
 *
 *  Created on: 12.10.2025
 *      Author: hocki
 */

#include "Application.h"
#include "Serialize.h"

namespace ApplicationNS
{
    // TaskNotification
    TaskNotification::TaskNotification(TaskHandle_t aTaskHandle, uint32_t aNotification)
        : mTaskHandle(aTaskHandle), mNotification(aNotification) {}

    TaskNotification::~TaskNotification() {}

    void TaskNotification::Notify(void)
    {
        xTaskNotify(mTaskHandle, mNotification, eSetBits);
    }

    void TaskNotification::NotifyFromISR(BaseType_t* apHigherPriorityTaskWoken)
    {
        xTaskNotifyFromISR(mTaskHandle, mNotification, eSetBits, apHigherPriorityTaskWoken);
    }



    // MessageReceiver
    MessageReceiver::MessageReceiver() {}

    MessageReceiver::~MessageReceiver()
    {
        mpMessageQueue = nullptr;
        mpNotification = nullptr;
    }

    void MessageReceiver::Init(MessageQueue* apMessageQueue, TaskNotification * apNotification)
    {
        assert(apMessageQueue != nullptr);
        assert(apNotification != nullptr);

        mpMessageQueue = apMessageQueue;
        mpNotification = apNotification;
    }

    void MessageReceiver::Init(MessageQueue* apMessageQueue, TaskHandle_t aTaskHandle, uint32_t aNotificationBitsToSet)
    {
        Init(apMessageQueue, new TaskNotification(aTaskHandle, aNotificationBitsToSet));
    }

    void MessageReceiver::NotifyMessage(const MessageNS::Message & arMessage)
    {
        mpMessageQueue->add(arMessage, 0);
        mpNotification->Notify();
    }


    TaskTimer::TaskTimer(uint32_t aTimerId, TickType_t aPeriod, bool aReload)
        : FreeRTOScpp::TimerClass(nullptr, aPeriod, aReload)
    {
        mTimerId = aTimerId;
    }

    TaskTimer::~TaskTimer()
    {
        /* Clear object pointers */
        delete mpTaskNotification;
        mpTaskNotification = nullptr;
        mpTaskTimerObjects = nullptr;
    }


    void TaskTimer::Init(tTaskTimerObjects* apTaskTimerObjects)
    {
        /* Ensure valid parameters */
        assert(apTaskTimerObjects != nullptr);
        assert(apTaskTimerObjects->mTaskHandle != nullptr);
        assert(apTaskTimerObjects->mpTaskMessagesQueue != nullptr);

        /* Store task timer objects */
        mpTaskTimerObjects = apTaskTimerObjects;

        /* Create task notification object */
        mpTaskNotification = new TaskNotification(
                mpTaskTimerObjects->mTaskHandle, mTaskNotificationMsgQueue);
    }

    void TaskTimer::timer(void)
    {
        if (mpTaskTimerObjects)
        {
            /* Create message for task */
            MessageNS::Message wMessage;
            wMessage.mSource      = MessageNS::tAddress::TASK_TIMER;
            wMessage.mDestination = MessageNS::tAddress::TASK;

            wMessage.mId   = MessageNS::tMessageId::MSG_EVENT_SW_TIMER_TIMEOUT;

            /* Serialize timer id into message payload */
            if (SerializeNS::SerializeData(mTimerId, wMessage.mPayload) == sizeof(mTimerId))
            {
                /* Set payload length */
                wMessage.mPayloadLength = sizeof(mTimerId);

                /* Add message to the task queue */
                mpTaskTimerObjects->mpTaskMessagesQueue->add(wMessage);

                /* Notify task */
                mpTaskNotification->Notify();
            }
            else
            {
                //TODO handle serialization error
            }

        }
    }


    // Task
    Task::Task(char const* apName, tTaskPriority aPriority, const uint32_t aStackSize)
        : FreeRTOScpp::TaskClassS<0>(apName, aPriority, aStackSize) {}

    void Task::Init(tTaskObjects* apTaskObjects)
    {
        /* Ensure that the task has been properly initialized with a valid task objects structure */
        assert(apTaskObjects != nullptr);
        assert(apTaskObjects->mpMessageQueue != nullptr);
        assert(apTaskObjects->mpCommunicationManager != nullptr);

        mpTaskObjects = apTaskObjects;
    }

    void Task::task(void)
    {
        uint32_t wNotificationValue;
        MessageNS::Message wMessage;

        /*
            * Task execution code
            */
        for (;;)
        {
            /* Wait to be notified */
            if (wait(0, 0xFFFFFFFF, &wNotificationValue) == pdPASS)
            {
                if ((wNotificationValue & mTaskNotificationMsgQueue) != 0)
                {
                    /* Clear notification bit */
                    wNotificationValue &= ~mTaskNotificationMsgQueue;

                    /* Process all messages in the queue */
                    while (mpTaskObjects->mpMessageQueue->pop(wMessage, 0))
                    {
                        /* Check if message is from timer */
                        if ((wMessage.mSource == MessageNS::tAddress::TASK_TIMER) &&
                            (wMessage.mDestination == MessageNS::tAddress::TASK))
                        {
                            ProcessIncomingTimerMessage(wMessage);
                        }
                        else
                        {
                            /* Process incoming message */
                            ProcessIncomingMessage(wMessage);
                        }

                        /* Allow other tasks to run */
                        yield();
                    }
                }

                /* Process any other notifications */
                if (wNotificationValue != 0)
                {
                    ProcessUnknownNotification(wNotificationValue);
                }
            }
        }
    }

    void Task::ProcessIncomingMessage(const MessageNS::Message &arMessage)
    {
#if (LOG_LEVEL_APPLI_NS == LOG_VERBOSE)
        LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_APPLICATION_NS,
            "%s::ProcessIncomingMessage() message from %s module",
            pcTaskGetName(NULL), MessageNS::AddressToString(arMessage.mSource));
#endif
        // to be implemented by derived class
    }

    void Task::ProcessIncomingTimerMessage(const MessageNS::Message &arMessage)
    {
        if (arMessage.mId == MessageNS::tMessageId::MSG_EVENT_SW_TIMER_TIMEOUT)
        {
            /* Retrieve timer ID from message payload */
            uint32_t wTimerId;
            if (SerializeNS::DeserializeData(arMessage.mPayload, &wTimerId) == sizeof(wTimerId))
            {
                ProcessTimerEvent(wTimerId);
            }
            else
            {
                //TODO handle serialization error
                LOG_WITH_REF(LOG_ERROR, LOG_LEVEL_APPLICATION_NS,
                        "%s::task() Error deserializing timer ID from message payload",
                        pcTaskGetName(NULL));
            }
        }
        else
        {
            //TODO handle unknown timer message ID
            LOG_WITH_REF(LOG_ERROR, LOG_LEVEL_APPLICATION_NS,
                    "%s::task() Unknown timer message ID %d",
                    pcTaskGetName(NULL), static_cast<int>(arMessage.mId));
        }
    }

    void Task::ProcessTimerEvent(const uint32_t aTimerId)
    {
    #if (LOG_LEVEL_APPLI_NS == LOG_VERBOSE)
        LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_APPLICATION_NS,
                "%s::ProcessTimerEvent() Timer id %d",
                pcTaskGetName(NULL), aTimerId);
    #endif
        // to be implemented by derived class
    }

    void Task::ProcessUnknownNotification(const uint32_t aNotificationValue)
    {
    #if (LOG_LEVEL_APPLI_NS == LOG_VERBOSE)
        LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_APPLICATION_NS,
                "%s::ProcessUnknownNotification() Notification value 0x%08X",
                pcTaskGetName(NULL), aNotificationValue);
    #endif
        // to be implemented by derived class
    }

};  /* end of namespace ApplicationNS */