/*
 * DateTime.cpp
 *
 *  Created on: 08.11.2026
 *      Author: hocki
 */
 #include "DateTime.h"

 namespace DateTimeNS
 {
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
    uint8_t DayOfWeek(uint8_t aDay, uint8_t aMonth, uint16_t aYear)
    {
        // Month table
        static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };

        aYear -= aMonth < 3;

        return  (aYear + aYear / 4 - aYear / 100 + aYear / 400 + t[aMonth - 1] + aDay) % 7;
    }

    /**
    * @brief Format an ISO-8601 UTC timestamp as ``DD.MM.YYYY HH:MM:SS``.
     *
    * The input is expected to start with ``YYYY-MM-DDTHH:MM:SS``; any
    * fractional seconds or UTC suffix are ignored. Returns false if either
    * the timestamp or destination buffer is invalid.
     */
    bool FormatIso8601Utc(const char* apTimestamp, char* apOutput, const size_t aOutputSize)
    {
        constexpr size_t kRequiredOutputSize = 20; // 19 visible characters plus NUL.
        if (apTimestamp == nullptr || apOutput == nullptr || aOutputSize < kRequiredOutputSize ||
            strlen(apTimestamp) < 19 || apTimestamp[4] != '-' || apTimestamp[7] != '-' ||
            apTimestamp[10] != 'T' || apTimestamp[13] != ':' || apTimestamp[16] != ':')
        {
            return false;
        }

        const auto readDigits = [apTimestamp](const size_t aStart, const size_t aCount, unsigned& arValue)
        {
            arValue = 0;
            for (size_t index = 0; index < aCount; ++index)
            {
                const char wDigit = apTimestamp[aStart + index];
                if (wDigit < '0' || wDigit > '9')
                {
                    return false;
                }
                arValue = arValue * 10 + static_cast<unsigned>(wDigit - '0');
            }
            return true;
        };

        unsigned wYear = 0;
        unsigned wMonth = 0;
        unsigned wDay = 0;
        unsigned wHour = 0;
        unsigned wMinute = 0;
        unsigned wSecond = 0;
        if (!readDigits(0, 4, wYear) || !readDigits(5, 2, wMonth) ||
            !readDigits(8, 2, wDay) || !readDigits(11, 2, wHour) ||
            !readDigits(14, 2, wMinute) || !readDigits(17, 2, wSecond) ||
            wMonth < 1 || wMonth > 12 || wHour > 23 || wMinute > 59 || wSecond > 59)
        {
            return false;
        }

        constexpr uint8_t kDaysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        unsigned wMaxDay = kDaysPerMonth[wMonth - 1];
        const bool wIsLeapYear = (wYear % 4 == 0) && ((wYear % 100 != 0) || (wYear % 400 == 0));
        if (wMonth == 2 && wIsLeapYear)
        {
            ++wMaxDay;
        }
        if (wDay < 1 || wDay > wMaxDay)
        {
            return false;
        }

        const int wLength = snprintf(apOutput, aOutputSize, "%02u.%02u.%04u %02u:%02u:%02u",
            wDay, wMonth, wYear, wHour, wMinute, wSecond);
        return wLength >= 0 && static_cast<size_t>(wLength) < aOutputSize;
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
    tDateTime GetCompileTime(void)
    {
        tDateTime wDateTime;

        /* Find month index (1-12) */
        char wCompMonth[4]{};
        memcpy(wCompMonth, __DATE__, 3);
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
     * @brief Returns the system uptime.
     *
     * @details
     * This function retrieves the system uptime and returns it as a tUptime structure,
     * which includes days, hours, minutes, and seconds.
     *
     * @return tUptime structure containing the system uptime.
     */
    tUptime GetUptime(void)
    {
        /* Local variable to hold uptime information */
        tUptime wUptime;
        /* Variable to hold uptime in seconds */
        uint64_t wUptimeSeconds = 0;

        /**
         * Calculate uptime from the ESP timer's signed 64-bit microsecond count.
         * Its range is about 292,000 years; tUptime's 15-bit day field is the
         * practical limit here (32,767 days, about 89.7 years).
         */
        wUptimeSeconds = static_cast<uint64_t>(esp_timer_get_time()) / 1000000ULL;

        /* Saturate at tUptime's maximum instead of allowing the day bit-field to wrap. */
        if (wUptimeSeconds > MAX_UPTIME_SECONDS)
        {
            wUptimeSeconds = MAX_UPTIME_SECONDS;
        }

        /* Calculate uptime in days, hours, minutes, and seconds */
        wUptime.wDays    = static_cast<uint32_t>(wUptimeSeconds / 86400ULL);
        wUptime.wHours   = static_cast<uint8_t>((wUptimeSeconds / 3600ULL) % 24ULL);
        wUptime.wMinutes = static_cast<uint8_t>((wUptimeSeconds / 60ULL  ) % 60ULL);
        wUptime.wSeconds = static_cast<uint8_t>( wUptimeSeconds % 60ULL);

        return wUptime;     // Return the uptime structure
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
    tDateTime DwordToDateTime(const uint32_t& arDword)
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
    uint32_t DateTimeToDword(const tDateTime& arDateTime)
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

    /**
     * @brief Checks if a time is within a given interval.
     *
     * @details
     * Returns true if the 'current' time is within the interval [start, end).
     * Handles intervals that cross midnight (e.g., start=22:00, end=06:00).
     *
     * @param arCurrentTime The current time (tTime).
     * @param arStartTime The start time of the interval (tTime).
     * @param arEndTime The end time of the interval (tTime).
     * @return True if current is in the interval, false otherwise.
     */
    bool IsTimeInInterval(const tTime& arCurrentTime, const tTime& arStartTime, const tTime& arEndTime)
    {
        uint16_t wCurrentMinutes = 60 * arCurrentTime.mHour + arCurrentTime.mMinute;
        uint16_t wStartMinutes   = 60 * arStartTime.mHour   + arStartTime.mMinute;
        uint16_t wEndMinutes     = 60 * arEndTime.mHour     + arEndTime.mMinute;

        if (wStartMinutes <= wEndMinutes)
        {
            /* Interval does not cross midnight */
            return (wCurrentMinutes >= wStartMinutes) && (wCurrentMinutes < wEndMinutes);
        }
        else
        {
            /* Interval crosses midnight */
            return (wCurrentMinutes >= wStartMinutes) || (wCurrentMinutes < wEndMinutes);
        }
    }

 }  // namespace DateTimeNS