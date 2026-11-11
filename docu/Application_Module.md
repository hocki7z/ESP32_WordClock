# Application Module

## Purpose

The `ApplicationNS` module provides the common FreeRTOS task infrastructure used by the application. It connects message routing, per-task queues, task notifications, and optional software timers. Application-specific work belongs in derived task classes; the module supplies the shared event-delivery and task-loop behavior.

Primary implementation files:

- `src/Application.h` — public types, class interfaces, and the task extension points.
- `src/Application.cpp` — notification, receiver, timer, and task-loop implementations.
- `src/Communication.h` and `src/Communication.cpp` — destination-address routing to registered message receivers.

## Main types

### `tTaskPriority`

An alias for `FreeRTOScpp::TaskPriority`, used when constructing a task.

### `MessageQueue` and `MessageQueueBase`

`MessageQueue` is a statically sized queue of `MessageNS::Message` objects with a capacity of 10. `MessageQueueBase` is its common base type, allowing timer/task context structures to refer to the queue without needing its capacity parameter.

### `TaskNotification`

Wraps a FreeRTOS task handle and notification value. `Notify()` signals the task from normal task context; `NotifyFromISR()` is for interrupt context and accepts the FreeRTOS higher-priority-task-woken parameter.

### `MessageReceiver`

Implements `CommunicationNS::NotificationCallback`. When `NotifyMessage()` receives a message, it enqueues it and notifies the associated task. `Init()` can be given an existing `TaskNotification`, or a task handle and notification bits.

**Ownership caution:** the task-handle `Init()` overload allocates a `TaskNotification` with `new`, but the current `MessageReceiver` destructor only clears its pointers. Unless ownership is changed in the implementation, that notification is not freed when the receiver is destroyed. Also ensure the receiver is unregistered or otherwise no longer reachable through `CommunicationManager` before destroying it.

### `tTaskObjects`

A context structure supplied to `Task::Init()`. It contains pointers to the communication manager and the task's incoming message queue. These are non-owning references: the task framework does not allocate or destroy either object. Keep them valid for as long as the task or a timer can access them.

### `tTaskTimerObjects`

A context structure used by `TaskTimer`, containing the target task handle and message queue. The base `Task` prepares this context in `Task::Init()`.

### `TaskTimer`

A `FreeRTOScpp::TimerClass` adapter. When its FreeRTOS timer expires, it creates a timeout `MessageNS::Message`, serializes the timer ID into its payload, puts the message on the task's queue, and notifies that task. It does not call task event handlers directly. The timer owns its internal `TaskNotification`; the task/message queue context it points to is borrowed and must outlive the active timer.

### `Task`

The base class for application tasks. It owns the optional `mpTaskTimer` and provides the processing loop plus overridable hooks:

- `ProcessIncomingMessage()` handles non-timer messages.
- `ProcessTimerEvent()` handles a timer event after its ID is decoded.
- `ProcessUnknownNotification()` handles notification bits not consumed as queue notifications.
- `SendMessage()` forwards a message through the configured communication manager.

The FreeRTOS entry point `task()` is a final override. Derived classes customize application behavior through the hooks rather than replacing the task entry point or main loop.

### `tTaskManagement`

Groups a task's address, task object, `tTaskObjects`, concrete queue, and receiver. In the application startup flow these objects are created and wired together before the task is triggered.

## Message and notification flow

1. A sender calls `CommunicationManager::SendMessage()` with a message containing a destination address.
2. The manager looks up the callback registered for that address and calls `NotifyMessage()` on the receiver.
3. The receiver adds the message to the destination task's queue and sets the message-queue notification bit (`mTaskNotificationMsgQueue`, bit 0).
4. The task wakes from `wait()` in `Task::ProcessTask()`.
5. If the queue bit is set, the base loop clears that bit and drains the queue. Timer-origin messages are sent to `ProcessIncomingTimerMessage()`, which validates the timer message ID, deserializes the timer ID, and calls `ProcessTimerEvent()`. Other messages go to `ProcessIncomingMessage()`.
6. Any remaining notification bits are passed to `ProcessUnknownNotification()`.

The notification is a wake-up signal; the message itself is held in the queue. Multiple queued messages can be handled after one wake-up because the task drains the queue.

## Optional task timer behavior

A timer is optional; the base class does not create one. A derived class creates `mpTaskTimer` when it needs a periodic or one-shot event, initializes it using the timer context prepared by `Task::Init()`, and chooses its own timer ID, period, and reload behavior.

When `ProcessTask()` starts, the base class starts `mpTaskTimer` if it is non-null. On expiry, the `TaskTimer` callback enqueues a timeout message. The normal message loop then dispatches the event to the derived class's `ProcessTimerEvent()` override. This keeps timer callbacks short and runs application-specific timer handling in the task's normal context.

Use `pdMS_TO_TICKS(milliseconds)` when specifying a period in milliseconds if the FreeRTOS configuration uses ticks. For a one-shot timer, set the reload argument to `false`; for a repeating timer, set it to `true`.

### Derived-task example

```cpp
void ExampleTask::Init(ApplicationNS::tTaskObjects* apTaskObjects)
{
    ApplicationNS::Task::Init(apTaskObjects);

    mpTaskTimer = new ApplicationNS::TaskTimer(
        mcTaskTimerId, pdMS_TO_TICKS(1000), true);
    mpTaskTimer->Init(&mTaskTimerObjects);
}

void ExampleTask::ProcessTimerEvent(const uint32_t aTimerId)
{
    if (aTimerId == mcTaskTimerId)
    {
        // Handle the periodic event in this task's context.
    }
}
```

Do not create a second timer for the same purpose in a derived-class member if it is already stored in the base-owned `mpTaskTimer`. The base `Task` destructor stops and deletes `mpTaskTimer`; derived destructors should only clean up additional timers or resources owned exclusively by the derived class. Do not explicitly call `Task::~Task()` from a derived destructor—C++ invokes it automatically.

## Initialization and lifetime requirements

Recommended setup order:

1. Construct the `CommunicationManager`.
2. Construct the task object and its message queue and receiver.
3. Fill in `tTaskObjects` with the manager and queue pointers.
4. Call the task's `Init()`. Derived `Init()` overrides should call `Task::Init()` before using the task timer context.
5. Register the receiver with the communication manager.
6. Trigger/start the task only after its task context and optional timer are initialized.

Keep the communication manager and message queue alive while the task or any timer may use them. A running task's `ProcessTask()` loop is infinite, so do not simply delete its C++ object while it is executing. If runtime shutdown is required, implement a coordinated stop/exit protocol, stop timers and callback sources, ensure no more messages can reach the receiver, then release the objects in a safe order. The FreeRTOScpp task wrapper also manages the underlying FreeRTOS task handle during destruction; do not separately delete the same handle.

## Practical constraints and checks

- Timer callbacks should enqueue and notify only; avoid lengthy application work in the FreeRTOS timer-service task.
- Check that queue insertion succeeds if message loss under queue saturation is significant to the application.
- The current `TaskTimer::timer()` checks its timer context pointer, but assumes its queue and notification pointers remain valid after successful initialization.
- `CommunicationManager` routes through registered callback pointers. Its lifetime and callback registration must be coordinated with receiver destruction.
- `MessageReceiver` and `tTaskObjects` store raw pointers; document ownership at the application level and avoid dangling references.
