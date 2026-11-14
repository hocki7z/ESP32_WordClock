#pragma once

#include <stdint.h>

#ifndef LOG_TASK_STATISTICS
#define LOG_TASK_STATISTICS (true)
#endif

#define TASK_DIAGNOSTICS_ENABLED ((USE_LOGGING == true) && (LOG_TASK_STATISTICS == true))

#if TASK_DIAGNOSTICS_ENABLED
#include <atomic>
#endif

namespace ApplicationNS
{
    struct QueueStatistics
    {
        uint32_t CurrentDepth = 0;
        uint32_t PeakDepth = 0;
        uint32_t FailedSends = 0;
        uint32_t Capacity = 0;
    };

    // Elapsed wall time includes preemption and blocking, not just CPU execution.
    struct TimingStatistics
    {
        uint32_t Count = 0;
        uint32_t MaxUs = 0;
        uint32_t LastUs = 0;
    };

    struct TaskStatistics
    {
        TimingStatistics IncomingMessage;
        TimingStatistics TimerHandler;
        TimingStatistics UnknownNotification;
        // Enqueue-attempt to handler dispatch; NOT periodic scheduled lateness.
        TimingStatistics TimerDeliveryDelayUs;
    };

#if TASK_DIAGNOSTICS_ENABLED
    static_assert(std::atomic<uint32_t>::is_always_lock_free,
        "Task diagnostics require lock-free 32-bit atomics");

    namespace TaskDiagnosticsDetail
    {
        // Independent relaxed fields provide race-free, approximate snapshots.
        inline void UpdateMax(std::atomic<uint32_t>& arMax, uint32_t aValue)
        {
            uint32_t wPrevious = arMax.load(std::memory_order_relaxed);
            while (wPrevious < aValue && !arMax.compare_exchange_weak(
                wPrevious, aValue, std::memory_order_relaxed, std::memory_order_relaxed))
            {
            }
        }

        class TimingCounters
        {
        public:
            // Saturate durations at UINT32_MAX microseconds; counts wrap naturally.
            void Record(int64_t aElapsedUs)
            {
                const uint32_t wElapsedUs = aElapsedUs <= 0 ? 0 :
                    (aElapsedUs > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(aElapsedUs));
                mLastUs.store(wElapsedUs, std::memory_order_relaxed);
                UpdateMax(mMaxUs, wElapsedUs);
                mCount.fetch_add(1, std::memory_order_relaxed);
            }

            TimingStatistics Snapshot() const
            {
                return {mCount.load(std::memory_order_relaxed),
                    mMaxUs.load(std::memory_order_relaxed),
                    mLastUs.load(std::memory_order_relaxed)};
            }

        private:
            std::atomic<uint32_t> mCount{0};
            std::atomic<uint32_t> mMaxUs{0};
            std::atomic<uint32_t> mLastUs{0};
        };
    }
#endif
}