/*
 * Application.cpp
 *
 *  Created on: 12.10.2025
 *      Author: hocki
 */

#include "Application.h"
#include "Serialize.h"

#if TASK_DIAGNOSTICS_ENABLED
#include <esp_timer.h>
#endif

namespace ApplicationNS
{
namespace
{
    // Queue items contain only value data; no allocation or ownership is transferred.
    MessageQueueItem MakeQueueItem(const MessageNS::Message& arMessage)
    {
        MessageQueueItem wItem{};
#if TASK_DIAGNOSTICS_ENABLED
        wItem.Payload = arMessage;
        if (arMessage.mSource == MessageNS::tAddress::TASK_TIMER)
        {
            wItem.EnqueueUs = esp_timer_get_time();
        }
#else
        wItem = arMessage;
#endif
        return wItem;
    }

    const MessageNS::Message& QueuePayload(const MessageQueueItem& arItem)
    {
#if TASK_DIAGNOSTICS_ENABLED
        return arItem.Payload;
#else
        return arItem;
#endif
    }
}

MessageQueueBase::MessageQueueBase(char const* name)
    : FreeRTOScpp::Queue<MessageQueueItem, mMessageQueueSize>(name)
{
}

// Sample after a successful send; a concurrent consumer can make this underestimate peak depth.
void MessageQueueBase::RecordSend(bool aSuccess, bool aFromISR)
{
#if TASK_DIAGNOSTICS_ENABLED
    if (aSuccess)
    {
        TaskDiagnosticsDetail::UpdateMax(mPeakDepth, aFromISR ? waiting_ISR() : waiting());
    }
    else
    {
        mFailedSends.fetch_add(1, std::memory_order_relaxed);
    }
#else
    (void)aSuccess;
    (void)aFromISR;
#endif
}

bool MessageQueueBase::Send(MessageNS::Message const& item, TickType_t time, bool aFront)
{
    const MessageQueueItem wItem = MakeQueueItem(item);
    const bool wSuccess = aFront ? xQueueSendToFront(queueHandle, &wItem, time) :
        xQueueSendToBack(queueHandle, &wItem, time);
    RecordSend(wSuccess, false);
    return wSuccess;
}

bool MessageQueueBase::SendFromISR(MessageNS::Message const& item, portBASE_TYPE& waswoken, bool aFront)
{
    const MessageQueueItem wItem = MakeQueueItem(item);
    const bool wSuccess = aFront ? xQueueSendToFrontFromISR(queueHandle, &wItem, &waswoken) :
        xQueueSendToBackFromISR(queueHandle, &wItem, &waswoken);
    RecordSend(wSuccess, true);
    return wSuccess;
}

// Only successful destructive reads replace consumer metadata; peeks leave it unchanged.
bool MessageQueueBase::Receive(MessageNS::Message& var, TickType_t time, bool aPeek)
{
    MessageQueueItem wItem{};
    const bool wSuccess = aPeek ? xQueuePeek(queueHandle, &wItem, time) :
        xQueueReceive(queueHandle, &wItem, time);
    if (wSuccess)
    {
        var = QueuePayload(wItem);
#if TASK_DIAGNOSTICS_ENABLED
        if (!aPeek) mLastDequeuedEnqueueUs = wItem.EnqueueUs;
#endif
    }
    return wSuccess;
}

bool MessageQueueBase::ReceiveFromISR(MessageNS::Message& var, portBASE_TYPE& waswoken, bool aPeek)
{
    MessageQueueItem wItem{};
    const bool wSuccess = aPeek ? xQueuePeekFromISR(queueHandle, &wItem) :
        xQueueReceiveFromISR(queueHandle, &wItem, &waswoken);
    if (wSuccess)
    {
        var = QueuePayload(wItem);
#if TASK_DIAGNOSTICS_ENABLED
        if (!aPeek) mLastDequeuedEnqueueUs = wItem.EnqueueUs;
#endif
    }
    return wSuccess;
}

bool MessageQueueBase::add(MessageNS::Message const& item, TickType_t time)
{
    return Send(item, time, false);
}

bool MessageQueueBase::push(MessageNS::Message const& item, TickType_t time)
{
    return Send(item, time, true);
}

bool MessageQueueBase::pop(MessageNS::Message& var, TickType_t time)
{
    return Receive(var, time, false);
}

bool MessageQueueBase::peek(MessageNS::Message& var, TickType_t time)
{
    return Receive(var, time, true);
}

bool MessageQueueBase::add_ISR(MessageNS::Message const& item, portBASE_TYPE& waswoken)
{
    return SendFromISR(item, waswoken, false);
}

bool MessageQueueBase::push_ISR(MessageNS::Message const& item, portBASE_TYPE& waswoken)
{
    return SendFromISR(item, waswoken, true);
}

bool MessageQueueBase::pop_ISR(MessageNS::Message& var, portBASE_TYPE& waswoken)
{
    return ReceiveFromISR(var, waswoken, false);
}

bool MessageQueueBase::peek_ISR(MessageNS::Message& var, portBASE_TYPE& waswoken)
{
    return ReceiveFromISR(var, waswoken, true);
}

QueueStatistics MessageQueueBase::GetStatistics() const
{
    QueueStatistics wStatistics{};
    wStatistics.CurrentDepth = waiting();
    wStatistics.Capacity = mMessageQueueSize;
#if TASK_DIAGNOSTICS_ENABLED
    wStatistics.PeakDepth = mPeakDepth.load(std::memory_order_relaxed);
    wStatistics.FailedSends = mFailedSends.load(std::memory_order_relaxed);
#endif
    return wStatistics;
}

int64_t MessageQueueBase::GetLastDequeuedEnqueueUs() const
{
#if TASK_DIAGNOSTICS_ENABLED
    return mLastDequeuedEnqueueUs;
#else
    return 0;
#endif
}

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
    if (mpMessageQueue->add(arMessage, 0))
    {
        mpNotification->Notify();
    }
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

/** @brief Returns the timer identifier
 *
 * @return The timer identifier associated with this TaskTimer instance.
*/
uint32_t TaskTimer::GetTimerId(void) const
{
    return mTimerId;
}

/**
 * @brief Callback function invoked when the timer expires.
 *
 * @details
 * This is final override of the FreeRTOScpp::TimerClass::timer function.
 * It is called when the timer expires. The function creates a timeout
 * message, serializes the timer ID into the message payload, adds the
 * message to the task's message queue, and notifies the associated task.
 */
void TaskTimer::timer(void)
{
    if (mpTaskTimerObjects)
    {
        /* Create message for task */
        MessageNS::Message wMessage{};
        wMessage.mSource      = MessageNS::tAddress::TASK_TIMER;
        wMessage.mDestination = MessageNS::tAddress::TASK;

        wMessage.mId   = MessageNS::tMessageId::MSG_EVENT_SW_TIMER_TIMEOUT;

        /* Serialize timer id into message payload */
        if (SerializeNS::SerializeData(mTimerId, wMessage.mPayload) == sizeof(mTimerId))
        {
            /* Set payload length */
            wMessage.mPayloadLength = sizeof(mTimerId);

            // Never block the shared timer service on a full task queue.
            if (mpTaskTimerObjects->mpTaskMessagesQueue->add(wMessage, 0))
            {
                mpTaskNotification->Notify();
            }
        }
        else
        {
            LOG_WITH_REF(LOG_ERROR, LOG_LEVEL_APPLICATION_NS,
                "%s::TaskTimer::timer() Error serializing timer ID %d into message payload",
                pcTaskGetName(NULL), mTimerId);
        }
    }
}


/**
 * @brief Implementation of the ApplicationNS::Task class
 *
 * @details
 * This class represents a FreeRTOS task that processes messages
 * and timer events. It inherits from FreeRTOScpp::TaskClassS<0>
 * and provides mechanisms for initializing task objects, handling
 * incoming messages, processing timer events, and managing notifications.
 *
 * @param apName The name of the task.
 * @param aPriority The priority of the task.
 * @param aStackSize The stack size for the task.
 */
Task::Task(char const* apName, tTaskPriority aPriority, const uint32_t aStackSize)
    : FreeRTOScpp::TaskClassS<0>(apName, aPriority, aStackSize),
      mConfiguredStackSize(aStackSize)
{
    // do nothing
}

uint32_t Task::GetConfiguredStackSize() const
{
    return mConfiguredStackSize;
}

TaskStatistics Task::GetStatistics() const
{
    TaskStatistics wStatistics{};
#if TASK_DIAGNOSTICS_ENABLED
    wStatistics.IncomingMessage = mIncomingMessageStatistics.Snapshot();
    wStatistics.TimerHandler = mTimerHandlerStatistics.Snapshot();
    wStatistics.UnknownNotification = mUnknownNotificationStatistics.Snapshot();
    wStatistics.TimerDeliveryDelayUs = mTimerDeliveryDelayUs.Snapshot();
#endif
    return wStatistics;
}

/**
 * @brief Destructor for the Task class.
 */
Task::~Task(void)
{
    /* Destgroy task timer if it exists */
    if (mpTaskTimer != nullptr)
    {
        /* Stop running timer */
        mpTaskTimer->stop();
        /*    and destroy it */
        delete mpTaskTimer;
        mpTaskTimer = nullptr;
    }
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

    /**
     * Init task timer
     *
     * The task timer will be created and initialized in the derived class,
     * as the timer period and reload behavior may vary based on specific task requirements.
     *
     * Example: Create a timer with a 1-second period and auto-reload behavior:
     *      mpTaskTimer = new TaskTimer(mTaskTimerId, pdMS_TO_TICKS(1000), true);
     *      mpTaskTimer->Init(&mTaskTimerObjects);
     */
    mTaskTimerObjects.mTaskHandle = this->getTaskHandle();
    mTaskTimerObjects.mpTaskMessagesQueue = mpTaskObjects->mpMessageQueue;
}

/**
 * @brief Wrapper for the FreeRTOScpp::TaskClassS<0>::task function.
 *
 * @details
 * This is final override of the task function from FreeRTOScpp::TaskClassS<0>.
 * It calls the ProcessTask method. Derived classes should implement the
 * ProcessTask method to define specific task behavior.
 */
void Task::task(void)
{
    ProcessTask();
}

/**
 * @brief Main task loop for processing notifications and messages.
 *
 * @details
 * This function implements the main execution loop for the task. It waits for notifications,
 * processes messages from the queue, distinguishes between timer and regular messages,
 * and handles unknown notifications. The loop runs indefinitely.
 */
void Task::ProcessTask(void)
{
    uint32_t wNotificationValue = 0;
    MessageNS::Message wMessage{};

    /* Start task timer if it exists */
    if (mpTaskTimer != nullptr)
    {
        mpTaskTimer->start();
    }

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
#if TASK_DIAGNOSTICS_ENABLED
                        const int64_t wStartUs = esp_timer_get_time();
#endif
                        ProcessIncomingMessage(wMessage);
#if TASK_DIAGNOSTICS_ENABLED
                        mIncomingMessageStatistics.Record(esp_timer_get_time() - wStartUs);
#endif
                    }

                    /* Allow other tasks to run */
                    yield();
                }
            }

            /* Process any other notifications */
            if (wNotificationValue != 0)
            {
#if TASK_DIAGNOSTICS_ENABLED
                const int64_t wStartUs = esp_timer_get_time();
#endif
                ProcessUnknownNotification(wNotificationValue);
#if TASK_DIAGNOSTICS_ENABLED
                mUnknownNotificationStatistics.Record(esp_timer_get_time() - wStartUs);
#endif
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
    /* To be implemented by derived class */

#if (LOG_LEVEL_APPLI_NS == LOG_VERBOSE)
    LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_APPLICATION_NS,
            "%s::ProcessIncomingMessage() message from %s module",
            pcTaskGetName(NULL), MessageNS::AddressToString(arMessage.mSource));
#endif
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
        // The decoder sees the whole fixed buffer, so validate the advertised length first.
        if (arMessage.mPayloadLength != sizeof(uint32_t) ||
            arMessage.mPayloadLength > sizeof(arMessage.mPayload))
        {
            LOG_WITH_REF(LOG_ERROR, LOG_LEVEL_APPLICATION_NS,
                "%s::task() Invalid timer payload length %u",
                pcTaskGetName(NULL), static_cast<unsigned>(arMessage.mPayloadLength));
            return;
        }

        /* Retrieve timer ID from message payload */
        uint32_t wTimerId = 0;
        if (SerializeNS::DeserializeData(arMessage.mPayload, &wTimerId) == sizeof(wTimerId))
        {
#if TASK_DIAGNOSTICS_ENABLED
            const int64_t wEnqueueUs = mpTaskObjects->mpMessageQueue->GetLastDequeuedEnqueueUs();
            const int64_t wDispatchUs = esp_timer_get_time();
            // This measures callback/enqueue-to-dispatch delay, not scheduled expiry lateness.
            mTimerDeliveryDelayUs.Record(wDispatchUs - wEnqueueUs);
            const int64_t wStartUs = esp_timer_get_time();
#endif
            ProcessTimerEvent(wTimerId);
#if TASK_DIAGNOSTICS_ENABLED
            mTimerHandlerStatistics.Record(esp_timer_get_time() - wStartUs);
#endif
        }
        else
        {
            LOG_WITH_REF(LOG_ERROR, LOG_LEVEL_APPLICATION_NS,
                    "%s::task() Error deserializing timer ID from message payload",
                    pcTaskGetName(NULL));
        }
    }
    else
    {
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
    /* To be implemented by derived class */

#if (LOG_LEVEL_APPLI_NS == LOG_VERBOSE)
    LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_APPLICATION_NS,
            "%s::ProcessTimerEvent() Timer id %d",
            pcTaskGetName(NULL), aTimerId);
#endif
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

/**
 * @brief Sends a message to the communication manager.
 *
 * @details
 * This function sends a message to the communication manager if it is available.
 *
 * @param arMessage The message to send.
 */
void Task::SendMessage(const MessageNS::Message &arMessage)
{
    if (mpTaskObjects && mpTaskObjects->mpCommunicationManager)
    {
        mpTaskObjects->mpCommunicationManager->SendMessage(arMessage);
    }
}

};  /* end of namespace ApplicationNS */