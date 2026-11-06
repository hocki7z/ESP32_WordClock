/*
 * RestartReason.cpp
 *
 *  Created on: 06.11.2026
 *      Author: hocki
 */

#include "RestartReason.h"

#include <MutexCPP.h>

#include "Global.h"


/* Log level for this module */
#define LOG_LEVEL_RESTART_REASON_NS    (LOG_DEBUG)


namespace RestartReasonNS
{
    /** @brief Restart reason, initialized to Unknown */
    static Type mRestartReason = Type::Unknown;

    /** @brief Mutex protecting mRestartReason from concurrent access across tasks */
    static FreeRTOScpp::RecursiveMutex mMutex("RestartReason");


    /** @brief Checks the reason for the last system reset and updates the restart reason accordingly */
    void CheckResetReason(void)
    {
        FreeRTOScpp::Lock wLock(mMutex);

        if (Settings.GetValue<bool>(ConfigNS::mKeyPendingFirmwareUpdate, false))
        {
            Settings.RemoveKey(ConfigNS::mKeyPendingFirmwareUpdate);
            Settings.IncreaseCounter(ConfigNS::mKeyCounterResetFirmwareUpdate);

            mRestartReason = Type::FirmwareUpdate;

            LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_RESTART_REASON_NS,
                "RestartReasonNS::CheckResetReason() Firmware update, %d times",
                Settings.GetCounter(ConfigNS::mKeyCounterResetFirmwareUpdate));
        }
        else
        {
            /* No pending firmware update, check the reason for the last system reset */
            esp_reset_reason_t wReason = esp_reset_reason();

            /* Update corresponding counters in settings */
            switch (wReason)
            {
                case ESP_RST_POWERON:   // Power-on reset
                    Settings.IncreaseCounter(ConfigNS::mKeyCounterResetPowerOn);
                    mRestartReason = Type::PowerOn;

                    LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_RESTART_REASON_NS,
                        "RestartReasonNS::CheckResetReason() Power-on reset, %d times",
                        Settings.GetCounter(ConfigNS::mKeyCounterResetPowerOn));
                    break;

                case ESP_RST_SW:        // Software reset
                    Settings.IncreaseCounter(ConfigNS::mKeyCounterResetSoftware);
                    mRestartReason = Type::Software;

                    LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_RESTART_REASON_NS,
                        "RestartReasonNS::CheckResetReason() Software reset, %d times",
                        Settings.GetCounter(ConfigNS::mKeyCounterResetSoftware));
                    break;

                case ESP_RST_WDT:       // Watchdog reset
                case ESP_RST_INT_WDT:   // Interrupt watchdog reset
                case ESP_RST_TASK_WDT:  // Task watchdog reset
                    Settings.IncreaseCounter(ConfigNS::mKeyCounterResetWatchdog);
                    mRestartReason = Type::Watchdog;

                    LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_RESTART_REASON_NS,
                        "RestartReasonNS::CheckResetReason() Watchdog reset, %d times",
                        Settings.GetCounter(ConfigNS::mKeyCounterResetWatchdog));
                    break;

                case ESP_RST_PANIC:     // Panic reset
                    Settings.IncreaseCounter(ConfigNS::mKeyCounterResetPanic);
                    mRestartReason = Type::Panic;

                    LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_RESTART_REASON_NS,
                        "RestartReasonNS::CheckResetReason() Panic reset, %d times",
                        Settings.GetCounter(ConfigNS::mKeyCounterResetPanic));
                    break;

                case ESP_RST_BROWNOUT:  // Brownout reset
                    Settings.IncreaseCounter(ConfigNS::mKeyCounterResetBrownout);
                    mRestartReason = Type::Brownout;

                    LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_RESTART_REASON_NS,
                        "RestartReasonNS::CheckResetReason() Brownout reset, %d times",
                        Settings.GetCounter(ConfigNS::mKeyCounterResetBrownout));
                    break;

                case ESP_RST_EXT:       // External reset
                case ESP_RST_DEEPSLEEP: // Deep sleep reset
                case ESP_RST_SDIO:      // SDIO reset
                case ESP_RST_UNKNOWN:   // unknown reset reason
                default:
                    /* Other reset reasons are not counted */

                    mRestartReason = Type::Unknown;
                    LOG_WITH_REF(LOG_VERBOSE, LOG_LEVEL_RESTART_REASON_NS,
                        "RestartReasonNS::CheckResetReason() Other reset reason: %d", wReason);

                    break;
            }
        }
    }

    /** @brief Get the current restart reason
     *
     * @return The current restart reason
     */
    Type GetRestartReason(void)
    {
        FreeRTOScpp::Lock wLock(mMutex);

        return mRestartReason;
    }

    /** @brief Converts the restart reason to a string
     *
     * @param aReason The restart reason to convert
     * @return A string representation of the restart reason
     */
    const char* ToString(Type aRestartReason)
    {
        switch (aRestartReason)
        {
            case Type::Unknown:        return "Unknown";
            case Type::PowerOn:        return "Power-on";
            case Type::Software:       return "Software request";
            case Type::Watchdog:       return "Watchdog timeout";
            case Type::Panic:          return "Panic";
            case Type::Brownout:       return "Brownout detection";
            case Type::FirmwareUpdate: return "Firmware update";
            default:                   return "Invalid reason";
        }
    };
}   // namespace RestartReasonNS