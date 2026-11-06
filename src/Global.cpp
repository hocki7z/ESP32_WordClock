/**
 * @file Global.cpp
 *
 * @brief Global definitions and includes for the ESP32 WordClock project.
 *
 * @details
 * This source file includes the global definitions and includes for the ESP32 WordClock project.
 *
 *  Created on: 06.11.2026
 *      Author: hocki
 */
#include "Global.h"


/** @brief Shared display-scene snapshot */
DisplaySceneSnapshot mDisplaySceneSnapshot;

/** @brief Shared list instance of discovered WiFi SSIDs */
SSIDList mSSIDList;

// /** @brief Shared restart reason */
// RestartReasonNS::Type mRestartReason;