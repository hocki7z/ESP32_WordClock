#pragma once

#include <Arduino.h>

namespace IdleCpuUsageNS
{
    /**
     * @brief Register per-core idle hooks and initialize the sampling baseline.
     * @return ESP_OK when hooks were registered on every CPU; otherwise an ESP error.
     */
    esp_err_t Initialize(void);

    /**
     * @brief Get estimated CPU usage since the previous sample.
     * @param arUsagePercent Receives the estimated percentage when available.
     * @return True when a full sampling interval is available; otherwise false.
     */
    bool GetUsagePercent(uint8_t& arUsagePercent);
}