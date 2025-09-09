/*
 * TimeManager.cpp
 *
 *  Created on: 08.09.2025
 *      Author: hocki
 */

#include <TimeLib64.h>

#include "TimeManager.h"

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
    /* Clean pointer */
    mpMinuteEventCallback = nullptr;
}

void TimeManager::Init(void)
{
    /* Initialize local time with compilation time */
    DateTimeNS::tDateTime wDateTime = GetCompileTime();
    SetDateTime(wDateTime);

    /* Init time variables */
    mPrevTime = GetDateTime();
}

void TimeManager::Loop(void)
{
    /* Set current time */
 	DateTimeNS::tDateTime wCurrTime = GetDateTime();

    /* Check if a minute event should be fired */
    if ((mPrevTime.mTime.mHour   != wCurrTime.mTime.mHour) ||
        (mPrevTime.mTime.mMinute != wCurrTime.mTime.mMinute))
    {
        /* Notify callback */
        if (mpMinuteEventCallback != nullptr)
        {
            mpMinuteEventCallback->NotifyDateTime(wCurrTime);
        }
    }

    /* Update previous time */
    mPrevTime = wCurrTime;
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


/**
 * @brief Set current time
 */
void TimeManager::SetDateTime(DateTimeNS::tDateTime aDateTime)
{
    /* Set new local time */
	setTime(aDateTime.mTime.mHour, aDateTime.mTime.mMinute, aDateTime.mTime.mSecond,
			aDateTime.mDate.mDay, aDateTime.mDate.mMonth, aDateTime.mDate.mYear);
}

/**
 * @brief Get current time
 */
DateTimeNS::tDateTime TimeManager::GetDateTime(void)
{
    DateTimeNS::tDateTime wDateTime;

	tmElements_t wTmElems;

	/* Parse current time */
	breakTime(now(), wTmElems);

	/* Fill time struct with the values */
	wDateTime.mTime.mSecond 	= wTmElems.Second;
	wDateTime.mTime.mMinute 	= wTmElems.Minute;
	wDateTime.mTime.mHour   	= wTmElems.Hour;
	//
	wDateTime.mDate.mDay 	  	= wTmElems.Day;
	wDateTime.mDate.mWeekDay 	= wTmElems.Wday;
	wDateTime.mDate.mMonth  	= wTmElems.Month;

	/* Convert tm.Year (offset from 1970) to the calendar year */
	wDateTime.mDate.mYear   	= tmYearToCalendar(wTmElems.Year);

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