/*
 * TimeManager.cpp
 *
 *  Created on: 08.09.2025
 *      Author: hocki
 */

#include <ESPNtpClient.h>

#include "Logger.h"

#include "TimeManager.h"


/* Log level for this module */
#define LOG_LEVEL               (LOG_DEBUG)

#define NTP_SERVER_NAME         PSTR("pool.ntp.org")
#define NTP_TIMEOUT             5000    // 5 sec
#define NTP_SYNC_PERIOD         600     // 10 min

#define TIME_ZONE               TZ_Europe_Berlin

#define LOG_DATE_TIME_FORMAT    "%02d/%02d/%04d %02d:%02d:%02d"


/**
 * @brief Constructor
 */
TimeManager::TimeManager(char const* apName, ApplicationNS::tTaskPriority aPriority, const uint32_t aStackSize)
    : ApplicationNS::Task(apName, aPriority, aStackSize)
{
    // do nothing
}

/**
 * @brief Destructor
 */
TimeManager::~TimeManager()
{
    /* Clear pointers */
    mpMinuteEventCallback = nullptr;
}

void TimeManager::Init(ApplicationNS::tTaskObjects* apTaskObjects)
{
    /* Initialize base class */
    ApplicationNS::Task::Init(apTaskObjects);

//    /* Initialize local time with compilation time */
//    DateTimeNS::tDateTime wCompileTime = GetCompileTime();
//    SetLocalTime(wCompileTime);

    /* Init time variables */
    mPrevTime = GetLocalTime();

    /* Register NTP sync events callback */
	NTP.onNTPSyncEvent(
        std::bind(&TimeManager::HandleNTPSyncEvent, this, std::placeholders::_1));

    /* Set time zone */
    NTP.setTimeZone(TIME_ZONE);
    /* Set sync parameters */
    NTP.setInterval(NTP_SYNC_PERIOD);
    NTP.setNTPTimeout(NTP_TIMEOUT);
//    NTP.setMinSyncAccuracy(5000);
//    NTP.settimeSyncThreshold(3000);

    /* Start NTP client */
    NTP.begin(NTP_SERVER_NAME, false);
}

void TimeManager::Loop(void)
{
    if (mNTPSyncEventTriggered)
    {
        /* Get current time */
 	    DateTimeNS::tDateTime wCurrTime = GetLocalTime();

        /* Check if a minute event should be fired */
        if ((mPrevTime.mTime.mHour   != wCurrTime.mTime.mHour) ||
            (mPrevTime.mTime.mMinute != wCurrTime.mTime.mMinute))
        {
            /* Notify callback */
            if (mpMinuteEventCallback != nullptr)
            {
                LOG(LOG_VERBOSE, "TimeManager::Loop() NotifyDateTime: " LOG_DATE_TIME_FORMAT,
                        wCurrTime.mDate.mDay,  wCurrTime.mDate.mMonth,  wCurrTime.mDate.mYear,
                        wCurrTime.mTime.mHour, wCurrTime.mTime.mMinute, wCurrTime.mTime.mSecond);

                mpMinuteEventCallback->NotifyDateTime(wCurrTime);
            }

            /* Update previous time after notification sent */
            mPrevTime = wCurrTime;
        }
    }
};

/**
 * @brief Register a callback for minute events
 */
void TimeManager::RegisterMinuteEventCallback(NotifyTimeCallback* apCallback)
{
    /* Check input parameter */
    if (apCallback != nullptr)
    {
        mpMinuteEventCallback = apCallback;
    }
}

void TimeManager::SetLocalTime(DateTimeNS::tDateTime aDateTime)
{
	SetLocalTime(aDateTime.mTime.mHour, aDateTime.mTime.mMinute, aDateTime.mTime.mSecond,
			    aDateTime.mDate.mDay,  aDateTime.mDate.mMonth,  aDateTime.mDate.mYear);
}

void TimeManager::SetLocalTime(uint8_t aHour, uint8_t aMinute, uint8_t aSecond, uint8_t aDay, uint8_t aMonth, uint16_t aYear)
{
    /* Get local time to keep DST (Daylight Saving Time) info */
    time_t wTime = time(nullptr);
    tm*  wpLocalTime = localtime(&wTime);

    /* Set date */
    wpLocalTime->tm_mday = aDay;
    wpLocalTime->tm_mon  = aMonth - 1;
    wpLocalTime->tm_year = aYear  - 1900;

    /* Set time */
    wpLocalTime->tm_hour = aHour;
    wpLocalTime->tm_min  = aMinute;
    wpLocalTime->tm_sec  = aSecond;

    /* Make new local time */
    time_t  wNewTime = mktime(wpLocalTime);
    timeval wNewTimeval = { .tv_sec = wNewTime };

    /* Set new local time */
    settimeofday(&wNewTimeval, nullptr);
}

/**
 * @brief Returns the compile date and time
 */
DateTimeNS::tDateTime TimeManager::GetCompileTime(void)
{
    DateTimeNS::tDateTime wDateTime;

    char compMon[4], *m;

    strncpy(compMon, __DATE__, 3);
    compMon[3] = '\0';
    m = strstr(DateTimeNS::mMonthsStr, compMon);

    wDateTime.mDate.mMonth  = ((m - DateTimeNS::mMonthsStr) / 3 + 1);
    wDateTime.mDate.mDay    = atoi(__DATE__ + 4);
    wDateTime.mDate.mYear   = atoi(__DATE__ + 7) /* - 1970*/;

    wDateTime.mTime.mHour   = atoi(__TIME__);
    wDateTime.mTime.mMinute = atoi(__TIME__ + 3);
    wDateTime.mTime.mSecond = atoi(__TIME__ + 6);

    return wDateTime;
}

DateTimeNS::tDateTime TimeManager::GetLocalTime(void)
{
    DateTimeNS::tDateTime wDateTime;

    /* Get local time */
    time_t wTime = time(NULL);
    tm*  wpLocalTime = localtime(&wTime);

    /* Get date */
    wDateTime.mDate.mDay    = wpLocalTime->tm_mday;
    wDateTime.mDate.mMonth  = wpLocalTime->tm_mon  + 1;
    wDateTime.mDate.mYear   = wpLocalTime->tm_year + 1900;

    /* Get time */
    wDateTime.mTime.mHour   = wpLocalTime->tm_hour;
    wDateTime.mTime.mMinute = wpLocalTime->tm_min;
    wDateTime.mTime.mSecond = wpLocalTime->tm_sec;

    return wDateTime;
}

void TimeManager::HandleNTPSyncEvent(NTPEvent_t aEvent)
{
    /* LOG */
    LOG(LOG_VERBOSE, "TimeManager::HandleNTPSyncEvent() Event %d", aEvent.event);

    /* Process event */
    switch (aEvent.event)
    {
        case timeSyncd:
        case partlySync:
        {
            /* Time successfully got from NTP server */
            LOG(LOG_DEBUG, "TimeManager::HandleNTPSyncEvent() Successful NTP sync at: %s",
                    NTP.getTimeDateString(NTP.getLastNTPSync()));

            mNTPSyncEventTriggered = true;

            uint8_t wHour, wMinute, wSecond;
            uint8_t wDay,  wMonth;
            uint16_t wYear;

            /* Parse time string and date strings */
            if ((sscanf(NTP.getTimeStr(), "%d:%d:%d", &wHour, &wMinute, &wSecond) == 3) &&      // 'HH:MM:SS'  , e.g. 00:23:56
                (sscanf(NTP.getDateStr(), "%d/%d/%d", &wDay,  &wMonth,  &wYear)   == 3))        // 'DD/MM/YYYY', e.g. 25/12/2023
            {
                /* Update local time */
                SetLocalTime(wHour, wMinute, wSecond, wDay,  wMonth, wYear);
            }
            else
            {
                LOG(LOG_ERROR, "TimeManager::HandleNTPSyncEvent() Parse time error");
            }
        }
            break;

        default:
            break;
    }
}