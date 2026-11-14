# Task statistics and stack sizing

## Purpose and current configuration

This document records the task-memory analysis from the device log supplied on
2026-11-14. The proposed sizes below are experimental starting points, not
validated worst-case requirements. No task allocation has been changed by this
analysis.

- [Task configuration](../src/Configuration.h) currently allocates 4096 bytes
  to each of the four application tasks, all at priority 1.
- [Statistics implementation](../src/main.cpp) reports shared heap health and
  stack watermarks every approximately 15 seconds when enabled.
- `LOG_TASK_STATISTICS` in [TaskDiagnostics.h](../src/TaskDiagnostics.h) is currently
   `false`. To collect reports, set it to
  `true` and use a build with `USE_LOGGING=true`, such as `nodemcu-32s_debug`.
  Both switches must be enabled. Do not redefine `USE_LOGGING` in the source;
  it is supplied by the [build configuration](../platformio.ini).

## What the report means

### Task columns

| Column | Meaning |
|---|---|
| `Task` | FreeRTOS task name. |
| `Priority` | Task scheduling priority at the time of the query. This is not CPU utilization. |
| `State` | Instantaneous running, ready, blocked, or suspended state; it can change before printing. |
| `Affinity` | CPU0, CPU1, or any core. This is affinity, not the core currently executing the task. |
| `StackB` | Configured allocation in bytes, retained by the application task or queried from Arduino for loopTask. |
| `MinFreeB` | Minimum-ever unused stack space observed since task creation, in bytes on this ESP-IDF platform. It is not current free stack. |
| `PeakUsedB` | Estimated lifetime peak used bytes: StackB minus MinFreeB. |
| `PeakUsed` | Estimated lifetime peak used percentage, rounded down to an integer. |

The watermark is derived from untouched stack-fill memory. Subtracting it from
the allocation estimates the observed peak stack usage; it does not prove that
all stack space required by every possible execution path has been exercised.
Reserved but untouched stack regions can also limit this estimate's accuracy.
Do not multiply the reported watermark by `sizeof(StackType_t)` on this platform.

Reports include the four long-lived application tasks and the Arduino `loopTask`.
They do **not** include per-task stack details for Wi-Fi, AsyncTCP, idle, or other
framework tasks. `systemTasks` counts all FreeRTOS tasks, including blocked and
suspended tasks; it is not the number executing simultaneously.

The installed precompiled Arduino/ESP-IDF framework does not enable the trace
facility required by `uxTaskGetSystemState()`. Panic snapshot APIs are not a safe
substitute for live enumeration. Reporting all task stacks would require a
suitable framework configuration and a concurrency-safe collection mechanism;
an application-only preprocessor define cannot add missing precompiled APIs.

### Shared heap fields

| Field | Meaning |
|---|---|
| `total` | Total heap size reported by `ESP.getHeapSize()`. |
| `free` | Current free heap reported by `ESP.getFreeHeap()`. |
| `minFree` | Minimum free heap reported by `ESP.getMinFreeHeap()`. |
| `largestBlock` | Largest allocatable block reported by `ESP.getMaxAllocHeap()`. |

Heap is shared across tasks. These fields do not attribute allocations to an
individual task. A large free-heap total does not guarantee that a single large
allocation will succeed; compare it with `largestBlock`.

## Queue and handler metrics

Each application task also reports its application-message queue and elapsed
handler timing. These measurements do not cover every framework queue or the
separate WebSite control-event queue.

| Metric | Meaning |
|---|---|
| Queue depth/capacity | Current queued messages and fixed capacity (10). |
| Queue observed peak | Maximum depth sampled after a successful send. Concurrent consumption can hide a transient peak; this is a lower-bound observation, not an exact historical maximum. |
| Queue failed sends | Lifetime count of unsuccessful queue insertions, including routed messages and timer events. A nonzero value warrants investigation. |
| `messageHandler` | Elapsed time in the normal-message handler. |
| `timerHandler` | Elapsed time in the timer-event handler, aggregated across that task's timer IDs. |
| `notificationBatch` | Elapsed time in the other-notification hook; for WebSite this may include an entire batch of control events. |
| `timerDeliveryDelay` | Time from a timer-message enqueue attempt until its valid timer handler is dispatched. |

Each timing row contains invocation `count`, `lastUs`, and lifetime `maxUs`.
Measurements include preemption, blocking, and logging inside a handler; they
are **not CPU execution time**. Waiting for notifications and the base loop's
post-message yield are not part of handler duration. Initialization is not timed.
Counters wrap after 2^32 events; durations saturate at 4294967295 microseconds.
Atomic fields make cross-task reads race-free, but the report is not a single
transactional snapshot. Queue depth and task state can change while printing.

`timerDeliveryDelay` does not include lateness before the timer callback runs.
It is **not lateness against the scheduled expiry**. True periodic lateness,
per-task CPU utilization, handler averages, and mutex contention are not collected.
Tracking scheduled deadlines reliably requires accounting for timer restarts and
resets; no approximation is labeled as scheduled lateness here.

### Queue-full behavior

Timer callbacks now attempt a nonblocking send. A full application queue causes
the timer event to be dropped rather than stalling the shared FreeRTOS timer
service. When diagnostics are enabled, that failure increments `failedSends`.
Normal routed sends remain nonblocking. Both paths notify the destination task
only after a successful insertion. This behavior applies even when diagnostics
are disabled. A dropped one-shot timer event is not retried automatically;
investigate queue pressure instead of treating dropped events as harmless.

### Instrumentation overhead and switch scope

The switch is shared across translation units because it changes the queue-item
layout. Never define it only inside one source file. Either edit its guarded
default in [TaskDiagnostics.h](../src/TaskDiagnostics.h) or supply a consistent
project-wide build definition.

Enabled builds add a timestamp to each queued message, atomic counters to tasks
and queues, and timestamp/counter work around dispatch. Fixed queue storage uses
`sizeof(DiagnosticEnvelope)` per slot instead of `sizeof(Message)` (40 slots
across the four application queues). Alignment affects the exact byte overhead.
The 15-second report itself also consumes CPU time, serial bandwidth, and loop
stack. Compare stack and timing results with this measurement overhead in mind.
Disabled builds omit envelopes, timing counters, and collection work; configured
stack metadata remains available. No reporting task or polling timer is added.

## Observed results and proposed first step

All sizes in the following table are bytes. The sample was supplied by the user;
its runtime duration and workload coverage were not established.

| Task | Configured stack | Minimum free observed | Estimated peak used | Proposed stack | Margin at the same observed peak | Potential saving |
|---|---:|---:|---:|---:|---:|---:|
| DisplayTask | 4096 | 2060 | 2036 | 3584 | 1548 | 512 |
| TimeManagerTask | 4096 | 2464 | 1632 | 3072 | 1440 | 1024 |
| WifiManagerTask | 4096 | 2100 | 1996 | 3584 | 1588 | 512 |
| WebSiteTask | 4096 | 1036 | 3060 | 4096 (unchanged) | 1036 | 0 |
| loopTask | Not established for the original sample | 5196 | Not calculated | No change proposed | Not calculated | 0 |

The application-task proposals reduce requested stack allocations by a total of
2048 bytes (2 KiB). Dynamically allocated stacks consume heap, so reducing them
can recover approximately this much heap, subject to allocator overhead and
rounding. The projected margins assume the same peak usage after rebuilding;
actual measurements may differ.

## How the suggested sizes were selected

First estimate peak usage:

$$
U = A - F
$$

where $A$ is the allocated stack and $F$ is the reported minimum free stack.

Then choose a safety margin $M$ and a convenient sizing increment $B$:

$$
S = B \left\lceil \frac{U + M}{B} \right\rceil
$$

The suggestions are conservative, manually selected values targeting roughly
1–1.5 KiB of remaining margin. They were **not** generated with one identical
margin for every task. For example, with $M = 1536$ and $B = 512$ for DisplayTask:

$$
U = 4096 - 2060 = 2036
$$

$$
S = 512 \left\lceil \frac{2036 + 1536}{512} \right\rceil = 3584
$$

The TimeManagerTask proposal retains 1440 bytes of margin rather than applying
that exact 1536-byte target. Using a strict 1536-byte margin and rounding up would
instead suggest 3584 bytes for that task. WifiManagerTask's proposed 3584 bytes
retain 1588 bytes of margin. WebSiteTask was left unchanged because its observed
remaining margin was already the smallest; this does not prove 4096 bytes is
sufficient for every workload.

**Neither a 512-byte sizing increment nor a 1–1.5 KiB margin is an ESP32 safety
requirement.** The increment is a tuning convenience. The margin is a heuristic,
and its adequacy depends on workload coverage, libraries, compiler settings, and
future changes.

## Configuration and validation workflow

1. Enable statistics and collect a baseline under representative load.
2. Change only one task's individual stack constant in
   [Configuration.h](../src/Configuration.h): `mDisplayTaskStackSize`,
   `mTimeManagerTaskStackSize`, `mWifiManagerTaskStackSize`, or
   `mWebSiteTaskStackSize`. Avoid lowering `mDefaultTaskStackSize` for every task.
3. Rebuild and upload the firmware. Task stack allocations take effect when the
   tasks are created; changing a constant does not resize a running task.
4. Repeat the workload checklist below and record the new minimum free stack,
   free heap, minimum free heap, and largest block. Keep build and test details
   with the measurements. Every reboot starts a new measurement history.
5. Accept the reduction only after adequate coverage and a deliberate margin
   decision. Revert or increase the allocation if headroom becomes insufficient
   or stack-related faults appear. Do not wait for a crash as the acceptance test.
6. Repeat for the next task. Retest after dependency, compiler, logging, or
   feature changes. Validate the production build too: debug measurements do not
   guarantee identical stack use in a release build.

### Workload checklist

- Cold boot with saved Wi-Fi credentials and without valid credentials.
- AP mode, Wi-Fi scanning, connection failures, disconnects, and reconnection.
- Successful NTP synchronization, DNS/network failures, synchronization timeouts,
  and timezone changes.
- All display modes, day/night transitions, and repeated setting changes.
- Multiple browser clients, page reloads, reconnects, and repeated settings saves.
- OTA upload, validation, failure paths where practical, and subsequent reboot.
- Extended operation with simultaneous network and display activity.
- Multi-reset detection and the startup work executed by Arduino `loopTask`.

Some asynchronous work runs in framework-owned tasks rather than in the
application task that initiated it. Healthy application watermarks therefore do
not establish that every system task has sufficient stack.

## Priority and Arduino loop recommendations

Keep application priorities at 1 unless timing measurements demonstrate missed
deadlines or responsiveness problems. Stack watermarks do not measure CPU load,
and raising priority does not reduce memory use. Raising a task's priority can
also delay other tasks if it does not block appropriately.

The Arduino loop stack is controlled by the framework, not the four application
stack constants. New reports query `getArduinoLoopTaskStackSize()` to show its
configured size. Although the original sample's 5196 bytes of unused stack
suggest potential savings, test startup, reset detection, and diagnostics before
choosing a smaller allocation.

The current statistics implementation reuses `loopTask` rather than allocating
a new reporting task. When enabled, it wakes approximately every 100 ms after
multi-reset detection to check the reporting interval. Reports are best-effort,
not hard real-time. When either logging switch is disabled, that periodic wakeup
and the statistics collection are compiled out.

## References

- [Application task configuration](../src/Configuration.h)
- [Statistics collection and Arduino loop](../src/main.cpp)
- [Shared diagnostics switch and counters](../src/TaskDiagnostics.h)
- [Queue instrumentation and handler dispatch](../src/Application.cpp)
- [Build environments and logging flags](../platformio.ini)
- [Application architecture](Application_Module.md)
- [ESP-IDF v4.4.7 FreeRTOS API reference](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/freertos.html)

Analysis and initial recommendations were prepared with AI assistance from the
project configuration and the user-provided device log. No hardware stress-test
results or validated minimum stack allocations are claimed.