/*
 * Application.cpp
 *
 *  Created on: 12.10.2025
 *      Author: hocki
 */

#include "Application.h"

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

    // NotificationTimer
    NotificationTimer::NotificationTimer(TaskHandle_t mTaskHandle, uint32_t mNotification, TickType_t aPeriod, bool aReload)
        : FreeRTOScpp::TimerClass(nullptr, aPeriod, aReload)
    {
        mpTaskNotification = new TaskNotification(mTaskHandle, mNotification);
    }

    NotificationTimer::~NotificationTimer()
    {
        delete mpTaskNotification;
    }

    void NotificationTimer::timer(void)
    {
        if (mpTaskNotification)
        {
            mpTaskNotification->Notify();
        }
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
                    while (mpTaskObjects->mpMessageQueue->pop(wMessage, 0))
                    {
                        /* Process incoming message */
                        ProcessIncomingMessage(wMessage);

                        /* Allow other tasks to run */
                        yield();
                    }
                }

                if ((wNotificationValue & mTaskNotificationTimer) != 0)
                {
                    /* Process timer event */
                    ProcessTimerEvent();
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

    void Task::ProcessTimerEvent(void)
    {
        // to be implemented by derived class
    }

};  /* end of namespace ApplicationNS */