/*
 * Application.h
 *
 *  Created on: 12.10.2025
 *      Author: hocki
 */

#ifndef SRC_APPLICATION_H_
#define SRC_APPLICATION_H_

#include <Arduino.h>

#include <FreeRTOScpp.h>
#include <TaskCPP.h>

#include "Logger.h"

/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)


namespace ApplicationNS
{
    /**
     * OS Tasks configurations
     */

    typedef FreeRTOScpp::TaskPriority tTaskPriority;

    /** @brief Default task stack size of tasks */
    static constexpr uint32_t    mDefaultTaskStackSize       = 2 * 1024;
    /** @brief Default priority of tasks */
    static constexpr tTaskPriority mDefaultTaskPriority      = FreeRTOScpp::TaskPrio_Low;

    static constexpr tTaskPriority mDisplayTaskPriority      = mDefaultTaskPriority;
    static constexpr uint32_t      mDisplayTaskStackSize     = mDefaultTaskStackSize;
    static constexpr const char*   mDisplayTaskName          = "DisplayTask";

    static constexpr tTaskPriority mTimeManagerTaskPriority  = mDefaultTaskPriority;
    static constexpr uint32_t      mTimeManagerTaskStackSize = mDefaultTaskStackSize;
    static constexpr const char*   mTimeManagerTaskName      = "TimeManagerTask";

    static constexpr tTaskPriority mWifiManagerTaskPriority  = mDefaultTaskPriority;
    static constexpr uint32_t      mWifiManagerTaskStackSize = mDefaultTaskStackSize;
    static constexpr const char*   mWifiManagerTaskName      = "WifiManagerTask";

    static constexpr tTaskPriority mWebSiteTaskPriority      = mDefaultTaskPriority;
    static constexpr uint32_t      mWebSiteTaskStackSize     = mDefaultTaskStackSize;
    static constexpr const char*   mWebSiteTaskName          = "WebSiteTask";

    /**
     * @brief Base class for all application tasks, based on
     *        dynamically created task wrapper FreeRTOScpp::TaskClassS<0>
     */
    class Task : public FreeRTOScpp::TaskClassS<0>
    {
    public:
        Task(char const* apName, tTaskPriority aPriority, const uint32_t aStackSize) :
            FreeRTOScpp::TaskClassS<0>(apName, aPriority, aStackSize)
        {
            this->give();
        }

        virtual void Init(void)
        {
            // to be implemented by derived class
        }

    protected:

        /* FreeRTOScpp::TaskClassS<0>::task */
        void task(void) override
        {
            // Get task name
            const char* taskName = pcTaskGetName(NULL);
            LOG(LOG_DEBUG, "ApplicationNS::Task::task() Task '%s' started", taskName);

            /*
             * Task execution code
             */
            for (;;)
            {
                yield();

                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    };

}; /* end of namespace ApplicationNS */

#endif /* SRC_APPLICATION_H_ */
