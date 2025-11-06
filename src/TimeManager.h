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
    TimeManager(char const* apName, ApplicationNS::tTaskPriority aPriority, const uint32_t aStackSize);
    virtual ~TimeManager();

    /* ApplicationNS::Task::Init */
    void Init(ApplicationNS::tTaskObjects* apTaskObjects) override;

private:

    /* Time source enumeration */
    enum tTimeSource
    {
        TIME_SOURCE_NONE = 0,       // Time not set
        TIME_SOURCE_COMPILE_TIME,   // Time set from compilation time
        TIME_SOURCE_NTP,            // Time set and managed from NTP server
        TIME_SOURCE_MANUAL,         // Time set manually by user
        //
        NB_TIME_SOURCES
    };

	/* Periodical timer for this task */
	ApplicationNS::tTaskTimerObjects mTimerObjects;
    ApplicationNS::TaskTimer* mpTimer;

    /* Current time source */
    tTimeSource mTimeSource = TIME_SOURCE_NONE;

    /* Last sent datatime */
    DateTimeNS::tDateTime mSentTime = DateTimeNS::tDateTime();

    uint8_t mNtpServer;
    uint8_t mTimeZone;

    /* ApplicationNS::Task::task() */
    void task(void) override;
    /* ApplicationNS::Task::ProcessTimerEvent() */
    void ProcessTimerEvent(const uint32_t aTimerId = 0) override;
    /* ApplicationNS::Task::ProcessIncomingMessage() */
    void ProcessIncomingMessage(const MessageNS::Message &arMessage) override;

    DateTimeNS::tDateTime GetLocalTime(void);
    DateTimeNS::tDateTime GetNtpTime(void);

    void SetLocalTimeFromNTP(void);

    void SetLocalTime(DateTimeNS::tDateTime aDateTime);
    void SetLocalTime(uint8_t aHour, uint8_t aMinute, uint8_t aSecond, uint8_t aDay, uint8_t aMonth, uint16_t aYear);

    void ResetTimeZone(void);


    void HandleNTPSyncEvent(NTPEvent_t aEvent);

    void SendTime(void);
};

#endif /* TIME_MANAGER_H_ */