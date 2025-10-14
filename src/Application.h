/*
 * Application.h
 *
 *  Created on: 12.10.2025
 *      Author: hocki
 */

#ifndef SRC_APPLICATION_H_
#define SRC_APPLICATION_H_

#include <Arduino.h>

#include <FreeRTOScpp.h>
#include <TaskCPP.h>
#include <QueueCPP.h>

#include "Logger.h"

#include "Message.h"
#include "Communication.h"


/* Log level for this module */
#define LOG_LEVEL_APPLICATION_NS    (LOG_VERBOSE)


namespace ApplicationNS
{
    /**
     * OS Tasks configurations
     */

     /* Task notification bits */
    static constexpr uint32_t mTaskNotificationMsgQueue      = 0x01; //binary: 00000000 00000000 00000000 00000001
    static constexpr uint32_t mTaskNotificationTimer         = 0x02; //binary: 00000000 00000000 00000000 00000010

    typedef FreeRTOScpp::TaskPriority tTaskPriority;

    /** @brief Default task stack size of tasks */
    static constexpr uint32_t    mDefaultTaskStackSize       = 2 * 1024;
    /** @brief Default priority of tasks */
    static constexpr tTaskPriority mDefaultTaskPriority      = FreeRTOScpp::TaskPrio_Low;

    static constexpr tTaskPriority mDisplayTaskPriority      = mDefaultTaskPriority;
    static constexpr uint32_t      mDisplayTaskStackSize     = mDefaultTaskStackSize;
    static constexpr const char*   mDisplayTaskName          = "DisplayTask";

    static constexpr tTaskPriority mTimeManagerTaskPriority  = mDefaultTaskPriority;
    static constexpr uint32_t      mTimeManagerTaskStackSize = mDefaultTaskStackSize;
    static constexpr const char*   mTimeManagerTaskName      = "TimeManagerTask";

    static constexpr tTaskPriority mWifiManagerTaskPriority  = mDefaultTaskPriority;
    static constexpr uint32_t      mWifiManagerTaskStackSize = mDefaultTaskStackSize;
    static constexpr const char*   mWifiManagerTaskName      = "WifiManagerTask";

    static constexpr tTaskPriority mWebSiteTaskPriority      = mDefaultTaskPriority;
    static constexpr uint32_t      mWebSiteTaskStackSize     = mDefaultTaskStackSize;
    static constexpr const char*   mWebSiteTaskName          = "WebSiteTask";

    class TaskNotification
    {
    public:
        TaskNotification(TaskHandle_t aTaskHandle, uint32_t aNotification) :
            mTaskHandle(aTaskHandle),
            mNotification(aNotification) {}

        virtual ~TaskNotification() {}

        void Notify(void)
        {
            xTaskNotify(mTaskHandle, mNotification, eSetBits);
        }

        void NotifyFromISR(BaseType_t* apHigherPriorityTaskWoken)
        {
            xTaskNotifyFromISR(mTaskHandle, mNotification, eSetBits, apHigherPriorityTaskWoken);
        }

    private:
        TaskHandle_t mTaskHandle;
        uint32_t     mNotification;

    };

    /** @brief Definition of the message queue */
    using MessageQueueBase = FreeRTOScpp::QueueTypeBase<MessageNS::Message>;

    static constexpr uint8_t mMessageQueueSize = 10;
    using MessageQueue = FreeRTOScpp::Queue<MessageNS::Message, mMessageQueueSize>;

    /** @brief Definition of message receiver */
    class MessageReceiver : public CommunicationNS::NotificationCallback
    {
    public:
        MessageReceiver() {};

        virtual ~MessageReceiver()
        {
            mpMessageQueue = nullptr;
            mpMessageQueue = nullptr;
        };

        void Init(MessageQueue* apMessageQueue, TaskNotification * apNotification)
        {
            assert(apMessageQueue != nullptr);
            assert(apNotification != nullptr);

            mpMessageQueue = apMessageQueue;
            mpNotification = apNotification;
        }

        void Init(MessageQueue* apMessageQueue, TaskHandle_t aTaskHandle, uint32_t aNotificationBitsToSet)
        {
            Init(apMessageQueue, new TaskNotification(aTaskHandle, aNotificationBitsToSet));
        }

        void NotifyMessage(const MessageNS::Message & arMessage)
        {
            mpMessageQueue->add(arMessage, 0);
            mpNotification->Notify();
        }

    private:
        MessageQueue*     mpMessageQueue = nullptr;
        TaskNotification* mpNotification = nullptr;
    };

    /** @brief Structure used to store all objects used by task initialization */
    struct tTaskObjects
    {
        /** @brief Communication manager */
        CommunicationNS::CommunicationManager* mpCommunicationManager;
        /** @brief Queue for incoming internal messages */
        MessageQueueBase* mpMessageQueue;
    };

    /**
     * @brief Base class for all application tasks, based on
     *        dynamically created task wrapper FreeRTOScpp::TaskClassS<0>
     */
    class Task : public FreeRTOScpp::TaskClassS<0>
    {
    public:
        Task(char const* apName, tTaskPriority aPriority, const uint32_t aStackSize) :
            FreeRTOScpp::TaskClassS<0>(apName, aPriority, aStackSize)
        {
            this->give();
        }

        virtual void Init(tTaskObjects* apTaskObjects)
        {
            /* Ensure that the task has been properly initialized with a valid task objects structure */
            assert(apTaskObjects != nullptr);
            assert(apTaskObjects->mpMessageQueue != nullptr);
            assert(apTaskObjects->mpCommunicationManager != nullptr);

            mpTaskObjects = apTaskObjects;
        }

    protected:
        tTaskObjects* mpTaskObjects;

        /* FreeRTOScpp::TaskClassS<0>::task */
        virtual void task(void) override
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

                            /* Yield should not be necessary, but cannot hurt eather */
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

        virtual void ProcessIncomingMessage(const MessageNS::Message &arMessage)
        {
            /* LOG */
#if (LOG_LEVEL_APPLI_NS == LOG_VERBOSE)
            LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_APPLICATION_NS,
                "%s::ProcessIncomingMessage() message from %s module",
                pcTaskGetName(NULL), MessageNS::AddressToString(arMessage.mSource));
#endif /* (LOG_LEVEL_APPLI_NS == LOG_VERBOSE) */

            // to be implemented by derived class
        }

        virtual void ProcessTimerEvent(void)
        {
            // to be implemented by derived class
        }
    };

}; /* end of namespace ApplicationNS */

#endif /* SRC_APPLICATION_H_ */
