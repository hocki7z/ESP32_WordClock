/*
 * ProductInfo.h
 *
 *  Created on: 02.11.2026
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>

namespace ProductInfoNS
{
    /** @brief Software version structure */
    struct tSoftwareVersion
    {
        uint8_t mMajor;     // Software version major component.
        uint8_t mMinor;     // Software version minor component.
        uint8_t mBuild;     // Software version build/patch component.

        /** @brief Checks if the software version is valid
         *
         *  @details
         *  Checks if the software version is valid by verifying that it is not equal
         *  to the invalid version (255.255.255). A valid software version is any version
         *  that is not equal to 255.255.255, which is considered an invalid version.
         *
         * @return true if the software version is valid, false if it is invalid (255.255.255)
         */
        bool IsValid(void) const
        {
            return (mMajor != 0xFF) || (mMinor != 0xFF) || (mBuild != 0xFF);
        }
    };

    /** @brief Product information record structure */
    struct __attribute__((packed)) Record
    {
        tSoftwareVersion mSoftwareVersion;  // Software version of the firmware.
        char mBuildDateTimeUtc[25];         // NUL-terminated UTC ISO-8601 build timestamp.
        char mBranchShort[81];              // NUL-terminated branch name, limited to 20 UTF-8 characters.
        char mCommitHashShort[9];           // NUL-terminated first 8 hexadecimal commit-hash characters.
    };

    const Record& GetRecord(void);

} // namespace ProductInfoNS