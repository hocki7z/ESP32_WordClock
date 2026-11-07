/*
 * RestartReason.h
 *
 *  Created on: 06.11.2026
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>

namespace RestartReasonNS
{
    /** @brief Restart reason enumeration */
    enum class Type : uint8_t
    {
        Unknown = 0,            ///< Unknown restart reason
        PowerOn,                ///< Restart due to power-on
        Software,               ///< Restart due to software request
        Watchdog,               ///< Restart due to watchdog timeout
        Panic,                  ///< Restart due to panic (assertion failure)
        Brownout,               ///< Restart due to brownout detection
        FirmwareUpdate          ///< Restart due to firmware update
    };


    void CheckResetReason(void);
    Type GetRestartReason(void);
    const char* ToString(Type aReason);

} // namespace RestartReasonNS