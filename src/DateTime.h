/*
 * DateTime.h
 *
 *  Created on: 21.09.2025
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>

#include <cstring>      // for strncpy, strstr
#include <cstdlib>      // for atoi


/** Defines to print a DateTimeNS::tDateTime value as string (for logging)
 *
 * You can use the below macros like this:
 *      LOG(LOG_DEBUG, "DateTime: " PRINTF_DATETIME_PATTERN, PRINTF_DATETIME_FORMAT(value));
 */
#define PRINTF_DATETIME_PATTERN             "%02u:%02u:%02u %02u.%02u.%04u"
#define PRINTF_DATETIME_FORMAT(datetime)    datetime.mTime.mHour, datetime.mTime.mMinute, datetime.mTime.mSecond, \
                                            datetime.mDate.mDay,  datetime.mDate.mMonth,  datetime.mDate.mYear


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
     * @brief Returns day of the week given the date via Tomohiko Sakamoto's Algorithm.
     *
     * @details
     * Calculates the weekday (0 = Monday, 1 = Tuesday, ..., 6 = Sunday) for a given date.
     * The algorithm is efficient and works for years > 1752. The arguments are passed by value.
     *
     * @link https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week
     *
     * @param aDay   Day of the month.
     * @param aMonth Month (1-12).
     * @param aYear  Year (> 1752).
     * @return Weekday (0 = Monday, 1 = Tuesday, ..., 6 = Sunday).
     */
    inline uint8_t DayOfWeek(uint8_t aDay, uint8_t aMonth, uint16_t aYear)
    {
        // Month table
        static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

        aYear -= aMonth < 3;

        return  (aYear + aYear / 4 - aYear / 100 + aYear / 400 + t[aMonth - 1] + aDay) % 7;
    }

    /**
     * @brief Returns the compile date and time as a DateTime structure.
     *
     * @details
     * This function parses the compiler macros __DATE__ and __TIME__ to construct a tDateTime structure
     * representing the date and time when the firmware was compiled. It extracts the month, day, year,
     * hour, minute, and second, and calculates the weekday using DayOfWeek. Useful for diagnostics,
     * logging, or displaying build information.
     *
     * @return tDateTime structure containing the compile date and time.
     */
    inline tDateTime CompileTime(void)
    {
        tDateTime wDateTime;

        /* Find month index (1-12) */
        char wCompMonth[4];
        strncpy(wCompMonth, __DATE__, 3);
        wCompMonth[3] = '\0';
        const char* wpMonthStr = strstr(mMonthsStr, wCompMonth);

        /* Parse __DATE__ ("Mmm dd yyyy") */
        wDateTime.mDate.mMonth = ((wpMonthStr - mMonthsStr) / 3 + 1);
        wDateTime.mDate.mDay   = atoi(__DATE__ + 4);
        wDateTime.mDate.mYear  = atoi(__DATE__ + 7); /* - 1970*/;

        /* Parse __TIME__ ("hh:mm:ss") */
        wDateTime.mTime.mHour   = atoi(__TIME__);
        wDateTime.mTime.mMinute = atoi(__TIME__ + 3);
        wDateTime.mTime.mSecond = atoi(__TIME__ + 6);

        /* Calculate weekday */
        wDateTime.mDate.mWeekDay = DayOfWeek(wDateTime.mDate.mDay,
                wDateTime.mDate.mMonth, wDateTime.mDate.mYear);

        return wDateTime;
    }

    /**
     * @brief Converts a double word value into a DateTime structure.
     *
     * @details
     * This function takes a 32-bit unsigned integer (dword) and extracts its encoded date and time fields
     * using bit shifting and masking. The result is returned as a tDateTime structure. The argument is passed
     * as a reference for efficiency. The function also calculates and sets the day of the week.
     *
     * @param arDword Reference to the dword value to convert.
     * @return Converted tDateTime structure.
     */
    inline tDateTime DwordToDateTime(const uint32_t& arDword)
    {
        tDateTime wDateTime;

        /* Convert double word into DataTime using bit shifting */
        wDateTime.mTime.mSecond = (arDword >> 00)  & 0x3F;
        wDateTime.mTime.mMinute = (arDword >> 06)  & 0x3F;
        wDateTime.mTime.mHour   = (arDword >> 12)  & 0x1F;
        wDateTime.mDate.mDay    = (arDword >> 17)  & 0x1F;
        wDateTime.mDate.mMonth  = (arDword >> 22)  & 0x0F;
        wDateTime.mDate.mYear   = ((arDword >> 26) & 0x3F) + mYearRangeStart;

        /* Set day of week */
        wDateTime.mDate.mWeekDay = DayOfWeek(
                wDateTime.mDate.mDay, wDateTime.mDate.mMonth, wDateTime.mDate.mYear);

        return wDateTime;
    }

    /**
     * @brief Converts a DateTime into double word value.
     *
     * @details
     * This function converts a DateTime structure into a 32-bit unsigned integer (dword)
     * using bit shifting. The argument is passed as a reference for efficiency.
     *
     * @param arDateTime Reference to the DateTime to convert.
     * @return Converted dword value.
     */
    inline uint32_t DateTimeToDword(const tDateTime& arDateTime)
    {
        /*
        * Dword (4 bytes)
        *  YYYYYYMM MMDDDDDS SSSSmmmm mmssssss
        *                               ------      Seconds :  range 0-59, 6 bits, mask 0x3F, offset 00
        *                        ---- --            Minutes :  range 0-59, 6 bits, mask 0x3F, offset 06
        *                  - ----                   Hours   :  range 0-23, 5 bits, mask 0x1F, offset 12
        *             -----                         Days    :  range 1-31, 5 bits, mask 0x1F, offset 17
        *        -- --                              Months  :  range 1-12, 4 bits, mask 0x0F, offset 22
        *  ------                                   Years   :  range 0-63, 6 bits, mask 0x3F, offset 26
        */

        uint32_t wDword = 0x00;

        /* Convert DataTime into double word using bit shifting */
        wDword |= (arDateTime.mTime.mSecond  & 0x3F) << 00;
        wDword |= (arDateTime.mTime.mMinute  & 0x3F) << 06;
        wDword |= (arDateTime.mTime.mHour    & 0x1F) << 12;
        wDword |= (arDateTime.mDate.mDay     & 0x1F) << 17;
        wDword |= (arDateTime.mDate.mMonth   & 0x0F) << 22;

        /* Set year only if within range */
        if (arDateTime.mDate.mYear >= mYearRangeStart)
        {
            wDword |= ((arDateTime.mDate.mYear - mYearRangeStart) & 0x3F) << 26;
        }

        return wDword;
    }

};  /* end of namespace DateTimeNS */
