/*
 * DateTime.h
 *
 *  Created on: 21.09.2025
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>

#include <cstdio>       // for snprintf
#include <cstring>      // for strncpy, strstr
#include <cstdlib>      // for atoi

#include <esp_timer.h>


/** Defines to print a DateTimeNS::tDateTime value as string (for logging)
 *
 * You can use the below macros like this:
 *      LOG(LOG_DEBUG, "DateTime: " PRINTF_DATETIME_PATTERN, PRINTF_DATETIME_FORMAT(value));
 */
#define PRINTF_DATETIME_PATTERN             "%02u:%02u:%02u %02u.%02u.%04u"
#define PRINTF_DATETIME_FORMAT(datetime)    datetime.mTime.mHour, datetime.mTime.mMinute, datetime.mTime.mSecond, \
                                            datetime.mDate.mDay,  datetime.mDate.mMonth,  datetime.mDate.mYear

/* Define maximum representable value: 32767 days, 23 hours, 59 minutes and 59 seconds */
#define MAX_UPTIME_SECONDS                  (32767ULL * 86400ULL + 86399ULL)

/* Define maximum representable value for tUptime without seconds: 32767 days, 23 hours and 59 minutes */
#define MASK_UPTIME_WITHOUT_SECONDS         0xFFFFFFC0U


namespace DateTimeNS
{
    /**
     * @brief Start year for date encoding and decoding.
     *
     * @details
     * The mYearRangeStart constant defines the base year (2000) used for encoding and decoding
     * years in the DateTime double word format. All year values are stored as an offset from this
     * start year to optimize storage and simplify calculations.
     */
    static constexpr uint16_t mYearRangeStart = 2000U;

    /**
     * @brief String containing abbreviated month names stored in program memory.
     *
     * @details
     * The mMonthsStr constant holds all 3-letter English month abbreviations ("JanFebMar...Dec")
     * in a single string. This is useful for formatting or displaying month names efficiently
     * on embedded systems.
     */
    static const char* mMonthsStr = "JanFebMarAprMayJunJulAugSepOctNovDec";

    /**
     * @brief Date structure for calendar information.
     *
     * @details
     * The tDate struct holds calendar date information including day, month, year, and weekday.
     * It provides operator overloads for equality and inequality checks, allowing easy comparison
     * of date values. This structure is used throughout the application to represent calendar dates.
     */
    typedef struct tDate
    {
        uint8_t  mDay;       /*!< Day of the month          1-31  */
        uint8_t  mMonth;     /*!< Months since January      1-12  */
        uint16_t mYear;      /*!< Years since 2000          0-99  */
        uint8_t  mWeekDay;   /*!< Day of the week           (from 0 to 6) */

        /* Operator == overload */
        bool operator==(const tDate& arOther) const
        {
            return ((mDay     == arOther.mDay)   &&
                    (mMonth   == arOther.mMonth) &&
                    (mYear    == arOther.mYear)  &&
                    (mWeekDay == arOther.mWeekDay));
        }

        /* Operator != overload */
        bool operator!=(const tDate& arOther) const
        {
            return !(*this == arOther);
        }
    } tDate;

    /**
     * @brief Time structure for clock information.
     *
     * @details
     * The tTime struct holds time information including hour, minute, and second.
     * It provides operator overloads for equality and inequality checks, allowing easy comparison
     * of time values. This structure is used throughout the application to represent clock times.
     */
    typedef struct tTime
    {
        uint8_t mHour;       /*!< Hours since midnight      0-23  */
        uint8_t mMinute;     /*!< Minutes after the hour    0-59  */
        uint8_t mSecond;     /*!< Seconds after the minute  0-59  */

        /* Operator == overload */
        bool operator==(const tTime& arOther) const
        {
            return ((mHour   == arOther.mHour)   &&
                    (mMinute == arOther.mMinute) &&
                    (mSecond == arOther.mSecond));
        }

        /* Operator != overload */
        bool operator!=(const tTime& arOther) const
        {
            return !(*this == arOther);
        }
    } tTime;

    /**
     * @brief DateTime structure combining date and time.
     *
     * @details
     * The tDateTime struct encapsulates both date (tDate) and time (tTime) information.
     * It provides operator overloads for equality and inequality checks, allowing easy comparison
     * of DateTime values. This structure is used throughout the application to represent
     * complete date and time stamps.
     */
    typedef struct tDateTime
    {
        tDate mDate;
        tTime mTime;

        /* Operator == overload */
        bool operator==(const tDateTime& arOther) const
        {
            return ((mDate == arOther.mDate) &&
                    (mTime == arOther.mTime));
        }

        /* Operator != overload */
        bool operator!=(const tDateTime& arOther) const
        {
            return !(*this == arOther);
        }
    } tDateTime;


    /**
     * @brief Compact uptime structure packed into a 32-bit allocation unit.
     *
     * @details
     * The fields use 15 + 5 + 6 + 6 = 32 bits. The maximum representable
     * normalized uptime is 32,767 days, 23 hours, 59 minutes, and 59 seconds
     * (about 89.7 years, or approximately 89 years and 8 months using the
     * average Gregorian year of 365.2425 days).
     */
    struct tUptime
    {
        uint32_t wDays    : 15;     // Storage range: 0-32,767 days
        uint32_t wHours   : 5;      // Storage range: 0-31; normalized uptime uses 0-23
        uint32_t wMinutes : 6;      // Storage range: 0-63; normalized uptime uses 0-59
        uint32_t wSeconds : 6;      // Storage range: 0-63; normalized uptime uses 0-59

        /**
         * @brief Convert the uptime structure to a 32-bit word.
         *
         * @return 32-bit representation of the uptime.
         */
        uint32_t ToWord(void) const
        {
            return ((static_cast<uint32_t>(wDays)    & 0x7FFFU) << 17) |
                   ((static_cast<uint32_t>(wHours)   & 0x001FU) << 12) |
                   ((static_cast<uint32_t>(wMinutes) & 0x003FU) << 6)  |
                   ( static_cast<uint32_t>(wSeconds) & 0x003FU);
        }

        /**
         * @brief Populate the uptime structure from a 32-bit word.
         *
         * @param aWord 32-bit representation of the uptime.
         */
        void FromWord(uint32_t aWord)
        {
            wDays    = (aWord >> 17) & 0x7FFF; // Extract 15 bits for days
            wHours   = (aWord >> 12) & 0x1F;   // Extract 5 bits for hours
            wMinutes = (aWord >> 6)  & 0x3F;   // Extract 6 bits for minutes
            wSeconds = aWord         & 0x3F;   // Extract 6 bits for seconds
        }

        /**
         * @brief Equality operator overload
         */
        bool operator==(const tUptime& arOther) const
        {
            return (ToWord() == arOther.ToWord());
        }

        /**
         * @brief Inequality operator overload
         */
        bool operator!=(const tUptime& arOther) const
        {
            return !(*this == arOther);
        }

        /**
         * @brief Greater-than operator overload
         */
        bool operator>(const tUptime& arOther) const
        {
            return (ToWord() > arOther.ToWord());
        }

        bool operator<(const tUptime& arOther) const
        {
            return (ToWord() < arOther.ToWord());
        }
    };

    /* Static assertion to ensure tUptime occupies exactly 4 bytes */
    static_assert(sizeof(tUptime) == sizeof(uint32_t), "tUptime must occupy exactly 4 bytes");


    uint8_t DayOfWeek(uint8_t aDay, uint8_t aMonth, uint16_t aYear);

    bool FormatIso8601Utc(const char* apTimestamp, char* apOutput, const size_t aOutputSize);

    tDateTime GetCompileTime(void);

    tUptime GetUptime(void);

    tDateTime DwordToDateTime(const uint32_t& arDword);

    uint32_t DateTimeToDword(const tDateTime& arDateTime);

    bool IsTimeInInterval(const tTime& arCurrentTime, const tTime& arStartTime, const tTime& arEndTime);

};  /* end of namespace DateTimeNS */
