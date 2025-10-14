/*
 * DateTime.h
 *
 *  Created on: 21.09.2025
 *      Author: hocki
 */

#ifndef SRC_DATETIME_H_
#define SRC_DATETIME_H_

#include <Arduino.h>

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
    static constexpr uint16_t mYearRangeStart = 2000U;

    static PGM_P mMonthsStr   = PSTR("JanFebMarAprMayJunJulAugSepOctNovDec");


    /**
     * @brief Date
     */
    typedef struct tDate
    {
        uint8_t  mDay;       /*!< Day of the month          1-31  */
        uint8_t  mMonth;     /*!< Months since January      1-12  */
        uint16_t mYear;      /*!< Years since 2000          0-99  */
        uint8_t  mWeekDay;   /*!< Day of the week           (from 0 to 6) */

        /* Operator == overload */
        bool operator==(const tDate& aOther) const
        {
            return ((mDay     == aOther.mDay)   &&
                    (mMonth   == aOther.mMonth) &&
                    (mYear    == aOther.mYear)  &&
                    (mWeekDay == aOther.mWeekDay));
        }

        /* Operator != overload */
        bool operator!=(const tDate& aOther) const
        {
            return !(*this == aOther);
        }
    } tDate;

    /**
     * @brief Time
     */
    typedef struct tTime
    {
        uint8_t mHour;       /*!< Hours since midnight      0-23  */
        uint8_t mMinute;     /*!< Minutes after the hour    0-59  */
        uint8_t mSecond;     /*!< Seconds after the minute  0-59  */

        /* Operator == overload */
        bool operator==(const tTime& aOther) const
        {
            return ((mHour   == aOther.mHour)   &&
                    (mMinute == aOther.mMinute) &&
                    (mSecond == aOther.mSecond));
        }

        /* Operator != overload */
        bool operator!=(const tTime& aOther) const
        {
            return !(*this == aOther);
        }
    } tTime;

    /**
     * @brief DateTime
     */
    typedef struct tDateTime
    {
        tDate mDate;
        tTime mTime;

        /* Operator == overload */
        bool operator==(const tDateTime& aOther) const
        {
            return ((mDate == aOther.mDate) &&
                    (mTime == aOther.mTime));
        }

        /* Operator != overload */
        bool operator!=(const tDateTime& aOther) const
        {
            return !(*this == aOther);
        }
    } tDateTime;


    /**
     * @brief Returns day of the week given the date
     *        via Tomohiko Sakamoto's Algorithm
     *
     * @link https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week
     *
     * @param aDay   Day of the month
     * @param aMonth Month (1-12)
     * @param aYear  Year  (> 1752)
     *
     * @return Weekday (0 = Monday, 1 = Tuesday...)
     */
    inline uint8_t DayOfWeek(uint8_t aDay, uint8_t aMonth, uint16_t aYear)
    {
        // Month table
        static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

        aYear -= aMonth < 3;

        return  (aYear + aYear / 4 - aYear / 100 + aYear / 400 + t[aMonth - 1] + aDay) % 7;
    }

    /**
     * @brief Converts a double word value into DateTime
     *
     * @param apDword[in] Dword value to convert
     *
     * @return converted DateTime
     */
    inline tDateTime DwordToDateTime(const uint32_t* apDword)
    {
        tDateTime wDateTime;

        uint32_t wDword = apDword[0];

        /* Convert double word into DataTime using bit shifting */
        wDateTime.mTime.mSecond =  (wDword >> 00)  & 0x3F;
        wDateTime.mTime.mMinute =  (wDword >> 06)  & 0x3F;
        wDateTime.mTime.mHour   =  (wDword >> 12)  & 0x1F;
        wDateTime.mDate.mDay    =  (wDword >> 17)  & 0x1F;
        wDateTime.mDate.mMonth  =  (wDword >> 22)  & 0x0F;
        wDateTime.mDate.mYear   =  ((wDword >> 26) & 0x3F) + mYearRangeStart;

        /* Set day of week */
        wDateTime.mDate.mWeekDay = DayOfWeek(
                wDateTime.mDate.mDay, wDateTime.mDate.mMonth, wDateTime.mDate.mYear);

        return wDateTime;
    }

    /**
     * @brief Converts a DateTime into double word value
     *
     * @param apDateTime[in] DateTime to convert
     *
     * @return converted dword
     */
    inline uint32_t DateTimeToDword(const tDateTime* apDateTime)
    {
        uint32_t wDWord = 0x00;

        /*
        * dword (4 bytes)
        *  00001111000011110000111100001111
        *                            ------    second  : range 0-59; 6 bits, mask 0x3F, offset 00
        *                      ------          minute  : range 0-59; 6 bits, mask 0x3F, offset 06
        *                 -----                hour    : range 0-23; 5 bits, mask 0x1F, offset 12
        *            -----                     day     : range 1-31; 5 bits, mask 0x1F, offset 17
        *        ----                          month   : range 1-12; 4 bits, mask 0x0F, offset 22
        *  ------                              year    : range 0-63; 6 bits, mask 0x3F, offset 26
        */

        /* Convert DataTime into double word using bit shifting */
        wDWord |= (apDateTime->mTime.mSecond  & 0x3F) << 00;
        wDWord |= (apDateTime->mTime.mMinute  & 0x3F) << 06;
        wDWord |= (apDateTime->mTime.mHour    & 0x1F) << 12;
        wDWord |= (apDateTime->mDate.mDay     & 0x1F) << 17;
        wDWord |= (apDateTime->mDate.mMonth   & 0x0F) << 22;
        wDWord |= ((apDateTime->mDate.mYear - mYearRangeStart) & 0x3F) << 26;

        return wDWord;
    }

};  /* end of namespace DateTimeNS */

#endif /* SRC_DATETIME_H_ */