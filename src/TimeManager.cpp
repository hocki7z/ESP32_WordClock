/*
 * TimeManager.cpp
 *
 *  Created on: 08.09.2025
 *      Author: hocki
 */

 #include <cstdio>      // for sscanf

#include <ESPNtpClient.h>

#include "Logger.h"
#include "Configuration.h"
#include "Serialize.h"
#include "Settings.hpp"

#include "Timezone.h"

#include "TimeManager.h"


/* Log level for this module */
#define LOG_LEVEL               (LOG_DEBUG)

#define NTP_TIMEOUT             5000    // 5 sec
#define NTP_SYNC_PERIOD         600     // 10 min

/* Periodical task timer ID */
static constexpr uint32_t mPeriodicalTaskTimerId = 0x01;


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
	/* Clean up task timer */
    if (mpTimer)
    {
		/* Stop running timer */
        mpTimer->stop();
		/*    and destroy it */
        delete mpTimer;
        mpTimer = nullptr;
    }
}

void TimeManager::Init(ApplicationNS::tTaskObjects* apTaskObjects)
{
    /* Initialize base class */
    ApplicationNS::Task::Init(apTaskObjects);

    /* Create periodical timer */
	mTimerObjects.mTaskHandle = this->getTaskHandle();
	mTimerObjects.mpTaskMessagesQueue = this->mpTaskObjects->mpMessageQueue;

    mpTimer = new ApplicationNS::TaskTimer(mPeriodicalTaskTimerId, 1000, true); // 1 sec period
	mpTimer->Init(&mTimerObjects);

//    /* Initialize local time with compilation time */
//    DateTimeNS::tDateTime wCompileTime = DateTimeNS::CompileTime();
//    SetLocalTime(wCompileTime);

    /* Init time variables */
    mSentTime = GetLocalTime();

    /* Register NTP sync events callback */
	NTP.onNTPSyncEvent(
        std::bind(&TimeManager::HandleNTPSyncEvent, this, std::placeholders::_1));

    /* Set time zone from settings */
    uint8_t wTimeZoneIndex = Settings.GetValue<uint8_t>(ConfigNS::mKeyTimeZone, ConfigNS::mDefaultTimeZone);
    NTP.setTimeZone(ConfigNS::mcTimezones[wTimeZoneIndex]);
   /* Set sync parameters */
    NTP.setInterval(NTP_SYNC_PERIOD);
    NTP.setNTPTimeout(NTP_TIMEOUT);
//    NTP.setMinSyncAccuracy(5000);
//    NTP.settimeSyncThreshold(3000);
}

void TimeManager::task(void)
{
    /* Start notification timer for this task */
    mpTimer->start();

    /* Execute base class task */
    ApplicationNS::Task::task();
};

void TimeManager::ProcessTimerEvent(const uint32_t aTimerId)
{
    /* Check NTP time sync */
    if (mNtpTimeSynced)
    {
        /* Get current time */
        DateTimeNS::tDateTime wCurrTime = GetLocalTime();

        /* Check if a minute event should be fired */
        if ((mSentTime.mTime.mHour   != wCurrTime.mTime.mHour) ||
            (mSentTime.mTime.mMinute != wCurrTime.mTime.mMinute))
        {
            /* Send changed time */
            SendTime();

            /* Update previous time after notification sent */
            mSentTime = wCurrTime;
        }
    }
}

void TimeManager::ProcessIncomingMessage(const MessageNS::Message &arMessage)
{
    LOG(LOG_VERBOSE, "TimeManager::ProcessIncomingMessage()");

    switch (arMessage.mId)
    {
        case MessageNS::tMessageId::MGS_EVENT_NTP_LASTSYNC_TIME:
        {
            /* Deserialize last sync time */
            uint32_t wDword;
            if (SerializeNS::DeserializeData(arMessage.mPayload, &wDword) == sizeof(wDword))
            {
                DateTimeNS::tDateTime wDateTime = DateTimeNS::DwordToDateTime(wDword);

                LOG(LOG_DEBUG, "TimeManager::ProcessIncomingMessage() NTP last sync time: " PRINTF_DATETIME_PATTERN,
                        PRINTF_DATETIME_FORMAT(wDateTime));

                /* Set local time to last sync time */
                SetLocalTime(wDateTime);

                /* Set NTP sync flag */
                mNtpTimeSynced = true;
            }
        }
            break;

        case MessageNS::tMessageId::MGS_STATUS_WIFI_STA_CONNECTED:
        {
            /* WiFi connected, start NTP sync */

            /* Get NTP server from settings */
            uint8_t wNtpServerIndex = Settings.GetValue<uint8_t>(ConfigNS::mKeyNtpServer, ConfigNS::mDefaultNtpServer);
            /* Start NTP client */
            NTP.begin(ConfigNS::mcNtpServerItems[wNtpServerIndex], false);
        }
            break;

        case MessageNS::tMessageId::MSG_EVENT_SETTINGS_CHANGED:
        {
            // Settings changed, re-read settings if needed
            LOG(LOG_DEBUG, "TimeManager::ProcessIncomingMessage() Settings changed");

            /* Set time zone from settings */
            uint8_t wTimeZoneIndex = Settings.GetValue<uint8_t>(ConfigNS::mKeyTimeZone, ConfigNS::mDefaultTimeZone);
            NTP.setTimeZone(ConfigNS::mcTimezones[wTimeZoneIndex]);

            /* Update local time */
            DateTimeNS::tDateTime wNTPTime = GetNtpTime();
            SetLocalTime(wNTPTime);

            /* Get current time */
            DateTimeNS::tDateTime wCurrTime = GetLocalTime();
            /* Send time */
            SendTime();

            /* Update previous time after notification sent */
            mSentTime = wCurrTime;
        }
            break;

        default:
            // do nothing
            break;
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

DateTimeNS::tDateTime TimeManager::GetLocalTime(void)
{
    DateTimeNS::tDateTime wDateTime;

    /* Get local time */
    time_t wTime = time(NULL);
    tm*  wpLocalTime = localtime(&wTime);

    /* Get date */
    wDateTime.mDate.mDay    = wpLocalTime->tm_mday;
    wDateTime.mDate.mMonth  = wpLocalTime->tm_mon + 1;
    wDateTime.mDate.mYear   = wpLocalTime->tm_year + 1900;

    /* Get time */
    wDateTime.mTime.mHour   = wpLocalTime->tm_hour;
    wDateTime.mTime.mMinute = wpLocalTime->tm_min;
    wDateTime.mTime.mSecond = wpLocalTime->tm_sec;

    return wDateTime;
}

DateTimeNS::tDateTime TimeManager::GetNtpTime(void)
{
    DateTimeNS::tDateTime wDateTime;

    /* Check if NTP sync ever happened */
    if (NTP.getLastNTPSync() != 0)
    {
        uint8_t wHour, wMinute, wSecond;
        uint8_t wDay,  wMonth;
        uint16_t wYear;

        /* Parse time string and date strings */
        if ((sscanf(NTP.getTimeStr(), "%d:%d:%d", &wHour, &wMinute, &wSecond) == 3) &&      // 'HH:MM:SS'  , e.g. 00:23:56
            (sscanf(NTP.getDateStr(), "%d/%d/%d", &wDay,  &wMonth,  &wYear)   == 3))        // 'DD/MM/YYYY', e.g. 25/12/2023
        {
            /* Set date */
            wDateTime.mDate.mDay    = wDay;
            wDateTime.mDate.mMonth  = wMonth;
            wDateTime.mDate.mYear   = wYear;

            /* Set time */
            wDateTime.mTime.mHour   = wHour;
            wDateTime.mTime.mMinute = wMinute;
            wDateTime.mTime.mSecond = wSecond;
        }
    }
    else
    {
        /* Return zero time if NTP sync never happened */
        memset(&wDateTime, 0, sizeof(wDateTime));
    }

    return wDateTime;
}

void TimeManager::SendTime(void)
{
    DateTimeNS::tDateTime wDateTime = GetLocalTime();

    LOG(LOG_VERBOSE, "TimeManager::SendTime() Time to send: " PRINTF_DATETIME_PATTERN,
            PRINTF_DATETIME_FORMAT(wDateTime));

    /* Create message */
    MessageNS::Message wMessage;
    wMessage.mSource = MessageNS::tAddress::TIME_MANAGER;
    wMessage.mDestination = MessageNS::tAddress::DISPLAY_MANAGER;

    wMessage.mId = MessageNS::tMessageId::MGS_EVENT_DATETIME_CHANGED;

    /* Serialize DateTime in message payload */
    uint32_t wDword = DateTimeNS::DateTimeToDword(wDateTime);
    if (SerializeNS::SerializeData(wDword, wMessage.mPayload) == sizeof(wDword))
    {
        /* Set payload length */
        wMessage.mPayloadLength = sizeof(wDword);
        /* Send message */
        mpTaskObjects->mpCommunicationManager->SendMessage(wMessage);
    }
    else
    {
        //TODO handle serialization error
    }
}

void TimeManager::HandleNTPSyncEvent(NTPEvent_t aEvent)
{
    /* LOG */
//    LOG(LOG_VERBOSE, "TimeManager::HandleNTPSyncEvent() Event %d", aEvent.event);

    /* Process event */
    switch (aEvent.event)
    {
        case timeSyncd:
        case partlySync:
        {
            /* Time successfully got from NTP server */
//            LOG(LOG_VERBOSE, "TimeManager::HandleNTPSyncEvent() Successful NTP sync at: %s",
//                    NTP.getTimeDateString(NTP.getLastNTPSync()));

            /* Create message */
            MessageNS::Message wMessage;
            wMessage.mSource = MessageNS::tAddress::TIME_MANAGER;
            wMessage.mDestination = MessageNS::tAddress::TIME_MANAGER;

            wMessage.mId = MessageNS::tMessageId::MGS_EVENT_NTP_LASTSYNC_TIME;

            /* Serialize NTP DateTime in message payload */
            DateTimeNS::tDateTime wNTPTime = GetNtpTime();
            uint32_t wDword = DateTimeNS::DateTimeToDword(wNTPTime);
            SerializeNS::SerializeData(wDword, wMessage.mPayload);

            /* Send message */
            mpTaskObjects->mpCommunicationManager->SendMessage(wMessage);
        }
            break;

        default:
            break;
    }
}