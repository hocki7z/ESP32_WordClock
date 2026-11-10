#pragma once

#include <Arduino.h>

namespace IdleCpuUsageNS
{
    /**
    * @brief Get CPU usage since the previous FreeRTOS runtime-statistics sample.
    * @param arUsagePercent Receives the percentage when a sampling interval is available.
     * @return True when a full sampling interval is available; otherwise false.
     */
    bool GetUsagePercent(uint8_t& arUsagePercent);
}