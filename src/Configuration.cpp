/*
 * Configuration.cpp
 *
 *  Created on: 10.07.2026
 *      Author: hocki
 */
#include "Configuration.h"


/** @brief Shared list instance of discovered WiFi SSIDs */
SSIDList mSSIDList;

/** @brief Shared display-scene snapshot */
DisplaySceneSnapshot mDisplaySceneSnapshot;

namespace ConfigNS
{
    /** @brief Shared restart reason, initialized to Unknown */
    tRestartReason::Type mRestartReason = tRestartReason::Unknown;
}