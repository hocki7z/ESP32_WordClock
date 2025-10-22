/*
 * Application.h
 *
 *  Created on: 12.10.2025
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>

#include <FreeRTOScpp.h>
#include <TaskCPP.h>
#include <QueueCPP.h>
#include <TimerCPP.h>

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
        TaskNotification(TaskHandle_t aTaskHandle, uint32_t aNotification);
        virtual ~TaskNotification();
        void Notify(void);
        void NotifyFromISR(BaseType_t* apHigherPriorityTaskWoken);

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
        MessageReceiver();
        virtual ~MessageReceiver();
        void Init(MessageQueue* apMessageQueue, TaskNotification * apNotification);
        void Init(MessageQueue* apMessageQueue, TaskHandle_t aTaskHandle, uint32_t aNotificationBitsToSet);
        void NotifyMessage(const MessageNS::Message & arMessage);

    private:
        MessageQueue*     mpMessageQueue = nullptr;
        TaskNotification* mpNotification = nullptr;
    };



    struct tTaskTimerObjects
    {
        /** @brief Task handle */
        TaskHandle_t mTaskHandle;

        /** @brief Task messages queue */
        MessageQueueBase* mpTaskMessagesQueue;
    };


    class TaskTimer : public FreeRTOScpp::TimerClass
    {
    public:
        TaskTimer(uint32_t aTimerId, TickType_t aPeriod, bool aReload);
        virtual ~TaskTimer();

        void Init(tTaskTimerObjects* apTaskTimerObjects);

    private:
        /** @brief Timer identifier */
        uint32_t mTimerId;

        tTaskTimerObjects* mpTaskTimerObjects = nullptr;
        TaskNotification*  mpTaskNotification = nullptr;

        void timer(void) override;
    };


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
        Task(char const* apName, tTaskPriority aPriority, const uint32_t aStackSize);
        virtual void Init(tTaskObjects* apTaskObjects);

    protected:
        tTaskObjects* mpTaskObjects;

        /* FreeRTOScpp::TaskClassS<0>::task */
        virtual void task(void) override;

        virtual void ProcessIncomingMessage(const MessageNS::Message &arMessage);
        virtual void ProcessIncomingTimerMessage(const MessageNS::Message &arMessage);
        virtual void ProcessTimerEvent(const uint32_t aTimerId = 0);
        virtual void ProcessUnknownNotification(const uint32_t aNotificationValue);
    };

}; /* end of namespace ApplicationNS */

