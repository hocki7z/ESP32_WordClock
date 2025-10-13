/*
 * TimeManager.h
 *
 *  Created on: 08.09.2025
 *      Author: hocki
 */
#ifndef TIME_MANAGER_H_
#define TIME_MANAGER_H_

#include <Arduino.h>
#include <ESPNtpClient.h>

#include "Application.h"

#include "DateTime.h"


class TimeManager : public ApplicationNS::Task
{
public:
    /**
     * Class for datetime notifications
     */
    class NotifyTimeCallback
    {
    public:
        /**
         * @brief Notify the callback of a new datetime
         */
        virtual void NotifyDateTime(const DateTimeNS::tDateTime aDateTime) = 0;
    };

public:
    TimeManager(char const* apName, ApplicationNS::tTaskPriority aPriority, const uint32_t aStackSize);
    virtual ~TimeManager();

    void Init(void);

    void Loop(void);

    // Register a callback for minute events
    void RegisterMinuteEventCallback(NotifyTimeCallback* apCallback);


private:

    NotifyTimeCallback* mpMinuteEventCallback = nullptr;

    DateTimeNS::tDateTime mPrevTime;

    bool mNTPSyncEventTriggered = false;

    DateTimeNS::tDateTime GetCompileTime(void);
    DateTimeNS::tDateTime GetLocalTime(void);

    void SetLocalTime(DateTimeNS::tDateTime aDateTime);
    void SetLocalTime(uint8_t aHour, uint8_t aMinute, uint8_t aSecond, uint8_t aDay, uint8_t aMonth, uint16_t aYear);

    void HandleNTPSyncEvent(NTPEvent_t aEvent);
};

#endif /* TIME_MANAGER_H_ */