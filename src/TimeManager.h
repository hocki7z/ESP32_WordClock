/*
 * TimeManager.h
 *
 *  Created on: 08.09.2025
 *      Author: hocki
 */

#include <Arduino.h>

namespace DateTimeNS
{
    static constexpr uint16_t mYearRangeStart = 2000U;

    PGM_P mMonthsStr   = PSTR("JanFebMarAprMayJunJulAugSepOctNovDec");
    PGM_P mPrintFormat = PSTR("%02u:%02u:%02u %02u/%02u/%04u");

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
    } tDateTime;
};


class TimeManager
{
public:
    TimeManager();
    virtual ~TimeManager();

    void Init(void);

    void Loop(void);


private:

    DateTimeNS::tDateTime mPrevTime;

    void SetDateTime(DateTimeNS::tDateTime aDateTime);
    DateTimeNS::tDateTime GetDateTime(void);

    DateTimeNS::tDateTime GetCompileTime(void);
};