/*
 * TimeManager.h
 *
 *  Created on: 08.09.2025
 *      Author: hocki
 */
#pragma once

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
    /* Last datetime sent to the display */
    DateTimeNS::tDateTime mSentTime;
    bool mHasSentTime = false;
    uint32_t mTestClockLastAdvanceMs = 0;

    /* True when production NTP sync or the test clock has initialized time. */
    bool mNtpTimeSynced = false;

    uint8_t mNtpServer;
    uint8_t mTimeZone;

    /* Override ApplicationNS::Task::ProcessTimerEvent() */
    void ProcessTimerEvent(const uint32_t aTimerId = 0) override;
    /* Override ApplicationNS::Task::ProcessIncomingMessage() */
    void ProcessIncomingMessage(const MessageNS::Message &arMessage) override;

    DateTimeNS::tDateTime GetLocalTime(void) const;
    DateTimeNS::tDateTime GetNtpTime(void);
#if WORDCLOCK_TEST_MODE
    bool IsNightModeTime(void) const;
#endif

    void SetLocalTimeFromNTP(void);

    void SetLocalTime(DateTimeNS::tDateTime aDateTime);
    void SetLocalTime(uint8_t aHour, uint8_t aMinute, uint8_t aSecond, uint8_t aDay, uint8_t aMonth, uint16_t aYear);

    void HandleNTPSyncEvent(NTPEvent_t aEvent);

    void SendTime(void);
};

