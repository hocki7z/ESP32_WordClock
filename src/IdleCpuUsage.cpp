#include "IdleCpuUsage.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#if defined(CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS) && CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
namespace
{
    static_assert(portNUM_PROCESSORS > 0, "At least one processor is required");

    TaskStatus_t* mpTaskStatusBuffer = nullptr;
    UBaseType_t mTaskStatusBufferCapacity = 0;
    uint32_t mPreviousIdleRuntime[portNUM_PROCESSORS]{};
    uint32_t mPreviousTotalRuntime = 0;
    bool mHasPreviousSample = false;

    bool CaptureRuntimeSample(uint32_t* apIdleRuntime, uint32_t& arTotalRuntime)
    {
        bool wSuccess = false;
        UBaseType_t wRequiredCapacity = uxTaskGetNumberOfTasks() + 4;

        for (uint8_t wAttempt = 0; (wAttempt < 3) && !wSuccess; ++wAttempt)
        {
            if (wRequiredCapacity > mTaskStatusBufferCapacity)
            {
                const size_t wBufferSize = sizeof(TaskStatus_t) * wRequiredCapacity;
                TaskStatus_t* wpNewBuffer = static_cast<TaskStatus_t*>(pvPortMalloc(wBufferSize));
                if (wpNewBuffer == nullptr)
                {
                    break;
                }

                if (mpTaskStatusBuffer != nullptr)
                {
                    vPortFree(mpTaskStatusBuffer);
                }
                mpTaskStatusBuffer = wpNewBuffer;
                mTaskStatusBufferCapacity = wRequiredCapacity;
            }

            const UBaseType_t wTaskCount = uxTaskGetSystemState(
                mpTaskStatusBuffer, mTaskStatusBufferCapacity, &arTotalRuntime);
            if ((wTaskCount == 0) || (wTaskCount >= mTaskStatusBufferCapacity))
            {
                wRequiredCapacity = (wTaskCount > mTaskStatusBufferCapacity)
                    ? wTaskCount + 4
                    : mTaskStatusBufferCapacity * 2;
                continue;
            }

            bool wFoundIdleTask[portNUM_PROCESSORS]{};
            for (UBaseType_t wCore = 0; wCore < portNUM_PROCESSORS; ++wCore)
            {
                const TaskHandle_t wIdleTaskHandle = xTaskGetIdleTaskHandleForCPU(wCore);
                for (UBaseType_t wTask = 0; wTask < wTaskCount; ++wTask)
                {
                    if (mpTaskStatusBuffer[wTask].xHandle == wIdleTaskHandle)
                    {
                        apIdleRuntime[wCore] = static_cast<uint32_t>(
                            mpTaskStatusBuffer[wTask].ulRunTimeCounter);
                        wFoundIdleTask[wCore] = true;
                        break;
                    }
                }
            }

            wSuccess = true;
            for (UBaseType_t wCore = 0; wCore < portNUM_PROCESSORS; ++wCore)
            {
                wSuccess = wSuccess && wFoundIdleTask[wCore];
            }
        }

        return wSuccess;
    }
}
#endif

namespace IdleCpuUsageNS
{
    bool GetUsagePercent(uint8_t& arUsagePercent)
    {
#if defined(CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS) && CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
        uint32_t wCurrentIdleRuntime[portNUM_PROCESSORS]{};
        uint32_t wCurrentTotalRuntime = 0;
        if (!CaptureRuntimeSample(wCurrentIdleRuntime, wCurrentTotalRuntime))
        {
            return false;
        }

        if (!mHasPreviousSample)
        {
            for (UBaseType_t wCore = 0; wCore < portNUM_PROCESSORS; ++wCore)
            {
                mPreviousIdleRuntime[wCore] = wCurrentIdleRuntime[wCore];
            }
            mPreviousTotalRuntime = wCurrentTotalRuntime;
            mHasPreviousSample = true;
            return false;
        }

        uint64_t wIdleRuntimeDelta = 0;
        for (UBaseType_t wCore = 0; wCore < portNUM_PROCESSORS; ++wCore)
        {
            /* Unsigned subtraction handles the 32-bit runtime-counter rollover. */
            wIdleRuntimeDelta += static_cast<uint32_t>(
                wCurrentIdleRuntime[wCore] - mPreviousIdleRuntime[wCore]);
            mPreviousIdleRuntime[wCore] = wCurrentIdleRuntime[wCore];
        }
        const uint32_t wElapsedRuntime = static_cast<uint32_t>(
            wCurrentTotalRuntime - mPreviousTotalRuntime);
        mPreviousTotalRuntime = wCurrentTotalRuntime;

        const uint64_t wTotalRuntimeAllCores =
            static_cast<uint64_t>(wElapsedRuntime) * portNUM_PROCESSORS;
        if (wTotalRuntimeAllCores == 0)
        {
            return false;
        }

        const uint64_t wBusyRuntime = (wIdleRuntimeDelta < wTotalRuntimeAllCores)
            ? (wTotalRuntimeAllCores - wIdleRuntimeDelta)
            : 0ULL;
        arUsagePercent = static_cast<uint8_t>(
            (wBusyRuntime * 100ULL + (wTotalRuntimeAllCores / 2ULL)) / wTotalRuntimeAllCores);
        return true;
#else
        (void)arUsagePercent;
        return false;
#endif
    }
}