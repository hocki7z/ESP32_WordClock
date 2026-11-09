#include "IdleCpuUsage.h"

#include <esp_freertos_hooks.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

namespace
{
    static_assert(portNUM_PROCESSORS > 0, "At least one processor is required");

    portMUX_TYPE mIdleCounterMux = portMUX_INITIALIZER_UNLOCKED;
    uint32_t mIdleHookEvents[portNUM_PROCESSORS]{};
    uint32_t mPreviousIdleHookEvents[portNUM_PROCESSORS]{};
    int64_t mPreviousSampleTimeUs = 0;
    bool mIsInitialized = false;

    bool IdleHookCallback(void)
    {
        const BaseType_t wCore = xPortGetCoreID();
        if ((wCore >= 0) && (wCore < portNUM_PROCESSORS))
        {
            portENTER_CRITICAL(&mIdleCounterMux);
            ++mIdleHookEvents[wCore];
            portEXIT_CRITICAL(&mIdleCounterMux);
        }

        /* Run once per idle-loop iteration; never force a busy loop. */
        return true;
    }
}

namespace IdleCpuUsageNS
{
    esp_err_t Initialize(void)
    {
        if (mIsInitialized)
        {
            return ESP_OK;
        }

        UBaseType_t wRegisteredCores = 0;
        for (UBaseType_t wCore = 0; wCore < portNUM_PROCESSORS; ++wCore)
        {
            const esp_err_t wError = esp_register_freertos_idle_hook_for_cpu(IdleHookCallback, wCore);
            if (wError != ESP_OK)
            {
                /* Undo partial registration so the sampler never reports a misleading aggregate. */
                for (UBaseType_t wRegisteredCore = 0; wRegisteredCore < wRegisteredCores; ++wRegisteredCore)
                {
                    esp_deregister_freertos_idle_hook_for_cpu(IdleHookCallback, wRegisteredCore);
                }

                return wError;
            }

            ++wRegisteredCores;
        }

        portENTER_CRITICAL(&mIdleCounterMux);
        for (UBaseType_t wCore = 0; wCore < portNUM_PROCESSORS; ++wCore)
        {
            mPreviousIdleHookEvents[wCore] = mIdleHookEvents[wCore];
        }
        portEXIT_CRITICAL(&mIdleCounterMux);

        mPreviousSampleTimeUs = esp_timer_get_time();
        mIsInitialized = true;
        return ESP_OK;
    }

    bool GetUsagePercent(uint8_t& arUsagePercent)
    {
        if (!mIsInitialized)
        {
            return false;
        }

        uint32_t wCurrentIdleHookEvents[portNUM_PROCESSORS]{};
        portENTER_CRITICAL(&mIdleCounterMux);
        for (UBaseType_t wCore = 0; wCore < portNUM_PROCESSORS; ++wCore)
        {
            wCurrentIdleHookEvents[wCore] = mIdleHookEvents[wCore];
        }
        portEXIT_CRITICAL(&mIdleCounterMux);

        const int64_t wCurrentTimeUs = esp_timer_get_time();
        const int64_t wElapsedTimeUs = wCurrentTimeUs - mPreviousSampleTimeUs;
        if (wElapsedTimeUs < 1000000LL)
        {
            return false;
        }

        uint64_t wIdleHookEventDelta = 0;
        for (UBaseType_t wCore = 0; wCore < portNUM_PROCESSORS; ++wCore)
        {
            /* Unsigned subtraction intentionally handles the 32-bit event counter rollover. */
            wIdleHookEventDelta += static_cast<uint32_t>(
                wCurrentIdleHookEvents[wCore] - mPreviousIdleHookEvents[wCore]);
            mPreviousIdleHookEvents[wCore] = wCurrentIdleHookEvents[wCore];
        }
        mPreviousSampleTimeUs = wCurrentTimeUs;

        const uint64_t wExpectedIdleEvents =
            (static_cast<uint64_t>(wElapsedTimeUs) * configTICK_RATE_HZ * portNUM_PROCESSORS) / 1000000ULL;
        if (wExpectedIdleEvents == 0)
        {
            return false;
        }

        /* Hook events approximate idle ticks; clamp interrupt-driven excess events to 100%. */
        uint64_t wIdlePercent = (wIdleHookEventDelta * 100ULL + (wExpectedIdleEvents / 2ULL)) /
            wExpectedIdleEvents;
        if (wIdlePercent > 100ULL)
        {
            wIdlePercent = 100ULL;
        }

        arUsagePercent = static_cast<uint8_t>(100ULL - wIdlePercent);
        return true;
    }
}