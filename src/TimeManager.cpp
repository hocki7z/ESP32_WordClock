/*
 * TimeManager.cpp
 *
 *  Created on: 08.09.2025
 *      Author: hocki
 */

#include <TimeLib64.h>
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
TimeManager::TimeManager()
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

void TimeManager::Init(void)
{
    /* Initialize local time with compilation time */
//    DateTimeNS::tDateTime wDateTime = GetCompileTime();
//    SetDateTime(wDateTime);

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

DateTimeNS::tDateTime TimeManager::ConvertTime(time_t aTime)
{
    DateTimeNS::tDateTime wDateTime;

    /* Parse given time */
    tmElements_t tm;
    breakTime(aTime, tm);

    /*Fill time struct with the values */
    wDateTime.mTime.mSecond 	= tm.Second;
    wDateTime.mTime.mMinute 	= tm.Minute;
    wDateTime.mTime.mHour   	= tm.Hour;
    //
    wDateTime.mDate.mDay 	  	= tm.Day;
    wDateTime.mDate.mWeekDay 	= tm.Wday;
    wDateTime.mDate.mMonth  	= tm.Month;

    /* Convert tm.Year (offset from 1970) to the calendar year */
    wDateTime.mDate.mYear   	= tmYearToCalendar(tm.Year);

    return wDateTime;
}

void TimeManager::SetDateTime(time_t aTime)
{
    DateTimeNS::tDateTime wDateTime = ConvertTime(aTime);

    /* Set new local time */
    SetDateTime(wDateTime);
}

/**
 * @brief Set current time
 */
void TimeManager::SetDateTime(DateTimeNS::tDateTime aDateTime)
{
	SetDateTime(aDateTime.mTime.mHour, aDateTime.mTime.mMinute, aDateTime.mTime.mSecond,
			    aDateTime.mDate.mDay,  aDateTime.mDate.mMonth,  aDateTime.mDate.mYear);
}

void TimeManager::SetDateTime(uint8_t aHour, uint8_t aMinute, uint8_t aSecond, uint8_t aDay, uint8_t aMonth, uint16_t aYear)
{
    LOG(LOG_DEBUG, "TimeManager::SetDateTime() New local datetime: " LOG_DATE_TIME_FORMAT,
            aDay, aMonth, aYear, aHour, aMinute, aSecond);

    /* Set new local time */
	setTime(aDay, aMonth, aYear, aHour, aMinute, aSecond);
}

/**
 * @brief Get current time
 */
DateTimeNS::tDateTime TimeManager::GetDateTime(void)
{
    /* Get current time */
    time_t wCurrTime = now();
    /* and convert it */
    DateTimeNS::tDateTime wDateTime = ConvertTime(wCurrTime);

	return wDateTime;
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

    time_t wUtcTime = time(NULL);
    tm*  wpCurrTime = localtime(&wUtcTime);

    wDateTime.mDate.mDay    = wpCurrTime->tm_mday;
    wDateTime.mDate.mMonth  = wpCurrTime->tm_mon + 1;
    wDateTime.mDate.mYear   = wpCurrTime->tm_year + 1900;

    wDateTime.mTime.mHour   = wpCurrTime->tm_hour;
    wDateTime.mTime.mMinute = wpCurrTime->tm_min;
    wDateTime.mTime.mSecond = wpCurrTime->tm_sec;

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

            //setTime(time(NULL));
            //SetDateTime(GetNTPLocalTime());

            /* Get NTP local time */
//            time_t wUtcTime = time(NULL);
//            tm*  wpCurrTime = localtime(&wUtcTime);
//
//            SetDateTime(wpCurrTime->tm_hour, wpCurrTime->tm_min, wpCurrTime->tm_sec,
//                    wpCurrTime->tm_mday, wpCurrTime->tm_mon + 1, wpCurrTime->tm_year + 1900);
        }
            break;

        default:
            break;
    }
}