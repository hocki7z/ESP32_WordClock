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
     * @typedef tTaskPriority
     * @brief Alias for FreeRTOScpp::TaskPriority, used to specify the priority of application tasks.
     *
     * @details
     * This type definition provides a convenient alias for the FreeRTOS C++ wrapper's TaskPriority type,
     * allowing tasks within the application to be assigned priorities in a consistent and readable manner.
     */
    typedef FreeRTOScpp::TaskPriority tTaskPriority;

    /**
     * @brief Notification bitmask for task message queue events.
     *
     * @details
     * The mTaskNotificationMsgQueue constant defines the bitmask used for task notifications
     * related to message queue events. It is used with FreeRTOS task notification APIs to signal
     * tasks when new messages are available in the queue, enabling event-driven task processing.
     * The value 0x01 represents the least significant bit, which is set or checked to indicate
     * a message queue notification.
     */
    static constexpr uint32_t mTaskNotificationMsgQueue = 0x01; //binary: 00000000 00000000 00000000 00000001

    /**
     * @brief Definition of the message queue for application tasks.
     *
     * @details
     * The message queue is used for inter-task communication, allowing tasks to exchange
     * MessageNS::Message objects. The queue size is defined by mMessageQueueSize, which
     * specifies the maximum number of messages that can be stored in the queue at any time.
     * MessageQueueBase provides the base type for the queue, while MessageQueue is a
     * type alias for a statically sized queue of MessageNS::Message objects.
     */
    static constexpr uint8_t mMessageQueueSize = 10;

    using MessageQueueBase = FreeRTOScpp::QueueTypeBase<MessageNS::Message>;
    using MessageQueue     = FreeRTOScpp::Queue<MessageNS::Message, mMessageQueueSize>;


    /**
     * @brief Definition of task notification.
     *
     * @details
     * The TaskNotification class provides a mechanism to signal tasks using FreeRTOS task notifications.
     * It encapsulates the task handle and notification value, and offers methods to notify the associated
     * task either from a normal context or from an interrupt service routine (ISR). This is typically used
     * for inter-task signaling, such as waking up a task or indicating that an event has occurred.
     */
    class TaskNotification
    {
    public:
        TaskNotification(TaskHandle_t aTaskHandle, uint32_t aNotification);
        virtual ~TaskNotification();

        void Notify(void);
        void NotifyFromISR(BaseType_t* apHigherPriorityTaskWoken);

    private:
        TaskHandle_t mTaskHandle;
        uint32_t   mNotification;
    };


    /**
     * @brief MessageReceiver class for handling message queue and task notifications.
     *
     * @details
     * The MessageReceiver class manages the reception of messages and notification signaling for tasks.
     * It provides initialization methods to set up the message queue and notification mechanism, either
     * by passing an existing TaskNotification object or by creating one from a task handle and notification bits.
     * The NotifyMessage method adds a message to the queue and notifies the associated task, enabling
     * efficient inter-task communication and event-driven processing.
     */
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


    /**
     * @brief Structure containing objects required for task timer initialization.
     *
     * @details
     * The tTaskTimerObjects structure holds references to the task handle and the message queue
     * used by a timer-driven task. It is used to initialize timer-related functionality, ensuring
     * that the timer can signal the correct task and deliver messages to the appropriate queue.
     */
    struct tTaskTimerObjects
    {
        /** @brief Task handle */
        TaskHandle_t mTaskHandle;

        /** @brief Task messages queue */
        MessageQueueBase* mpTaskMessagesQueue;
    };

    /**
     * @brief TaskTimer class for managing timer-driven task signaling.
     *
     * @details
     * The TaskTimer class extends FreeRTOScpp::TimerClass to provide timer functionality for tasks.
     * It stores a timer identifier, manages references to the associated task and its message queue,
     * and uses a TaskNotification object to signal the task when the timer expires. The timer callback
     * method creates and sends timeout messages to the task, enabling event-driven processing based on timer events.
     */
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


    /**
     * @brief Structure containing objects required for task initialization.
     *
     * @details
     * The tTaskObjects structure holds references to the communication manager and the message queue
     * used by a task. It is used to initialize task-related functionality, ensuring that the task
     * can communicate effectively within the application framework.
     */
    struct tTaskObjects
    {
        /** @brief Communication manager */
        CommunicationNS::CommunicationManager* mpCommunicationManager;
        /** @brief Queue for incoming internal messages */
        MessageQueueBase* mpMessageQueue;
    };


    /**
     * @brief Base class for application tasks.
     *
     * @details
     * The Task class extends FreeRTOScpp::TaskClassS<0> to provide a framework for application tasks.
     * It includes methods for initialization, message processing, timer event handling, and notification
     * processing. Derived classes should implement specific task logic by overriding the provided virtual methods.
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

