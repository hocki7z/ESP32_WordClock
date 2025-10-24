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
/**
 *
 * Implementation of ApplicationNS::TaskNotification class
 *
 */
TaskNotification::TaskNotification(TaskHandle_t aTaskHandle, uint32_t aNotification)
    : mTaskHandle(aTaskHandle), mNotification(aNotification)
{
    // do nothing
}

TaskNotification::~TaskNotification()
{
    // do nothing
}

/**
 * @brief Notifies the associated task from a normal (non-ISR) context.
 *
 * @details
 * This function sends a notification to the task referenced by mTaskHandle using the FreeRTOS
 * xTaskNotify API. The notification value (mNotification) is set using eSetBits mode, which sets
 * the bits specified in mNotification in the receiving task's notification value.
 * This is typically used to signal or wake up a task from another task context.
 */
void TaskNotification::Notify(void)
{
    xTaskNotify(mTaskHandle, mNotification, eSetBits);
}

/**
 * @brief Notifies the associated task from an ISR context.
 *
 * @details
 * This function sends a notification to the task referenced by mTaskHandle using the FreeRTOS
 * xTaskNotifyFromISR API. The notification value (mNotification) is set using eSetBits mode.
 * The apHigherPriorityTaskWoken parameter is used to indicate if a higher priority task was woken
 * as a result of the notification, allowing for proper context switching after the ISR.
 *
 * @param apHigherPriorityTaskWoken Pointer to a variable that will be set to pdTRUE if a higher priority task was woken.
 */
void TaskNotification::NotifyFromISR(BaseType_t* apHigherPriorityTaskWoken)
{
    xTaskNotifyFromISR(mTaskHandle, mNotification, eSetBits, apHigherPriorityTaskWoken);
}


/**
 *
 * Implementation of the ApplicationNS::MessageReceiver class
 *
 */
MessageReceiver::MessageReceiver()
{
    // do nothing
}

MessageReceiver::~MessageReceiver()
{
    mpMessageQueue = nullptr;
    mpNotification = nullptr;
}

/**
 * @brief Initializes the MessageReceiver with a message queue and a TaskNotification object.
 *
 * @details
 * This function sets up the MessageReceiver by assigning the provided message queue and
 * TaskNotification pointer. Both parameters must be valid and non-null.
 *
 * @param apMessageQueue Pointer to the message queue to be used.
 * @param apNotification Pointer to the TaskNotification object for task signaling.
 */
void MessageReceiver::Init(MessageQueue* apMessageQueue, TaskNotification * apNotification)
{
    /* Ensure valid parameters */
    assert(apMessageQueue != nullptr);
    assert(apNotification != nullptr);

    /* Store parameters */
    mpMessageQueue = apMessageQueue;
    mpNotification = apNotification;
}

/**
 * @brief Initializes the MessageReceiver with a message queue and task notification parameters.
 *
 * @details
 * This overload creates a new TaskNotification object using the provided task handle and
 * notification bits, then delegates initialization to the main Init function.
 *
 * @param apMessageQueue Pointer to the message queue to be used.
 * @param aTaskHandle Handle of the task to be notified.
 * @param aNotificationBitsToSet Notification bits to set when signaling the task.
 */
void MessageReceiver::Init(MessageQueue* apMessageQueue, TaskHandle_t aTaskHandle, uint32_t aNotificationBitsToSet)
{
    Init(apMessageQueue, new TaskNotification(aTaskHandle, aNotificationBitsToSet));
}

/**
 * @brief Adds a message to the queue and notifies the associated task.
 *
 * @details
 * This function adds the provided message to the message queue and then sends a notification
 * to the associated task using the TaskNotification object. This mechanism is used to signal
 * the task that a new message is available for processing.
 *
 * @param arMessage The message to be added to the queue.
 */
void MessageReceiver::NotifyMessage(const MessageNS::Message & arMessage)
{
    mpMessageQueue->add(arMessage, 0);
    mpNotification->Notify();
}


/**
 *
 * Implementation of the ApplicationNS::TastTimes class
 *
 */
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

/**
 * @brief Initializes the TaskTimer with the provided timer objects.
 *
 * @details
 * This function sets up the TaskTimer by storing the provided timer objects pointer and
 * creating a TaskNotification object for signaling the associated task. All required pointers
 * in the timer objects structure must be valid and non-null.
 *
 * @param apTaskTimerObjects Pointer to the structure containing task handle and message queue.
 */
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

/**
 * @brief Timer callback function for TaskTimer.
 *
 * @details
 * This function is called when the timer expires. It creates a timeout message, serializes
 * the timer ID into the message payload, adds the message to the task's message queue, and
 * notifies the associated task. If serialization fails, error handling should be implemented.
 */
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


/**
 *
 * Implementation of the ApplicationNS::Task class
 *
 */
Task::Task(char const* apName, tTaskPriority aPriority, const uint32_t aStackSize)
    : FreeRTOScpp::TaskClassS<0>(apName, aPriority, aStackSize)
{
    // do nothing
}

/**
 * @brief Initializes the Task with the provided task objects structure.
 *
 * @details
 * This function sets up the Task by storing the provided task objects pointer.
 * It asserts that all required pointers in the structure are valid and non-null.
 *
 * @param apTaskObjects Pointer to the structure containing message queue and communication manager.
 */
void Task::Init(tTaskObjects* apTaskObjects)
{
    /* Ensure that the task has been properly initialized with a valid task objects structure */
    assert(apTaskObjects != nullptr);
    assert(apTaskObjects->mpMessageQueue != nullptr);
    assert(apTaskObjects->mpCommunicationManager != nullptr);

    /* Store task objects pointer */
    mpTaskObjects = apTaskObjects;
}

/**
 * @brief Main task loop for processing notifications and messages.
 *
 * @details
 * This function implements the main execution loop for the task. It waits for notifications,
 * processes messages from the queue, distinguishes between timer and regular messages,
 * and handles unknown notifications. The loop runs indefinitely.
 */
void Task::task(void)
{
    uint32_t wNotificationValue;
    MessageNS::Message wMessage;

    /* Task execution code */
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

/**
 * @brief Processes an incoming message.
 *
 * @details
 * This function handles messages that are not timer-related. The default implementation
 * logs the message source. Derived classes should override this function to provide
 * specific message handling logic.
 *
 * @param arMessage The incoming message to process.
 */
void Task::ProcessIncomingMessage(const MessageNS::Message &arMessage)
{
#if (LOG_LEVEL_APPLI_NS == LOG_VERBOSE)
    LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_APPLICATION_NS,
            "%s::ProcessIncomingMessage() message from %s module",
            pcTaskGetName(NULL), MessageNS::AddressToString(arMessage.mSource));
#endif
    // to be implemented by derived class
}

/**
 * @brief Processes an incoming timer message.
 *
 * @details
 * This function handles messages from the timer. It deserializes the timer ID from the
 * message payload and calls ProcessTimerEvent. If deserialization fails, it logs an error.
 *
 * @param arMessage The timer message to process.
 */
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

/**
 * @brief Processes a timer event.
 *
 * @details
 * This function is called when a timer event is received. The default implementation
 * logs the timer ID. Derived classes should override this function to provide
 * specific timer event handling logic.
 *
 * @param aTimerId The ID of the timer that triggered the event.
 */
void Task::ProcessTimerEvent(const uint32_t aTimerId)
{
#if (LOG_LEVEL_APPLI_NS == LOG_VERBOSE)
    LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_APPLICATION_NS,
            "%s::ProcessTimerEvent() Timer id %d",
            pcTaskGetName(NULL), aTimerId);
#endif
    // to be implemented by derived class
}

/**
 * @brief Processes unknown notification values.
 *
 * @details
 * This function handles notification values that do not correspond to known events.
 * The default implementation logs the notification value. Derived classes should
 * override this function to provide specific handling logic.
 *
 * @param aNotificationValue The unknown notification value to process.
 */
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