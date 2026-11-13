/**
 * @file Global.h
 *
 * @brief Global definitions and includes for the ESP32 WordClock project.
 *
 * @details
 * This header file contains global definitions, includes, and configurations
 * that are used throughout the ESP32 WordClock project. It serves as a central
 * point for including necessary libraries and defining constants that are shared
 * across multiple source files.
 *
 *  Created on: 06.11.2026
 *      Author: hocki
 */
#pragma once

#include "Configuration.h"
#include "Settings.hpp"

#include "Logger.h"
#include "Serialize.h"

#include "DisplayLayout.h"
#include "SSIDList.h"
#include "RestartReason.h"

/* Accelerated test clock: one simulated minute per interval, only during configured night hours. */
#define WORDCLOCK_TEST_MODE         (true)      // Enable accelerated test clock
/* Milliseconds between simulated minute changes; can be overridden with a build define. */
#define WORDCLOCK_TEST_INTERVAL     5000        // 5 seconds per simulated minute


/** @brief Shared display-scene snapshot */
extern DisplaySceneSnapshot mDisplaySceneSnapshot;

/** @brief Shared list instance; access entries only through synchronized methods */
extern SSIDList mSSIDList;

// /** @brief Shared restart reason */
// extern RestartReasonNS::Type mRestartReason;
