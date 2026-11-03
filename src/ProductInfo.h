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
            return (mMajor != 0xFF) ||
                   (mMinor != 0xFF) ||
                   (mBuild != 0xFF);
        }

        /** @brief Equality operator overload
         *
         * @param other The other software version to compare with
         * @return true if the software versions are equal, false otherwise
         */
        bool operator==(const tSoftwareVersion& other) const
        {
            return (mMajor == other.mMajor) && 
                   (mMinor == other.mMinor) && 
                   (mBuild == other.mBuild);
        }

        /** @brief Inequality operator overload
         *
         * @param other The other software version to compare with
         * @return true if the software versions are not equal, false otherwise
         */
        bool operator!=(const tSoftwareVersion& other) const
        {
            return !(*this == other);
        }

        /** @brief Less-than operator overload
         *
         * @param other The other software version to compare with
         * @return true if this software version is less than the other, false otherwise
         */
        bool operator<(const tSoftwareVersion& other) const
        {
            if (mMajor != other.mMajor)
                return mMajor < other.mMajor;
            if (mMinor != other.mMinor)
                return mMinor < other.mMinor;
            return mBuild < other.mBuild;
        }

        /** @brief Greater-than operator overload
         *
         * @param other The other software version to compare with
         * @return true if this software version is greater than the other, false otherwise
         */
        bool operator>(const tSoftwareVersion& other) const
        {
            // Check if != and not <
            return *this != other && !(*this < other);
        }
    };

    /** @brief Product information record structure */
    struct __attribute__((packed)) Record
    {
        uint8_t mMagic[8];                  // Record identifier bytes: "WCPRODIN".
        uint8_t mFormatVersion;             // Binary record layout version.
        tSoftwareVersion mSoftwareVersion;  // Software version of the firmware.
        char mBuildDateTimeUtc[25];         // NUL-terminated UTC ISO-8601 build timestamp.
        char mBranchShort[81];              // NUL-terminated branch name, limited to 20 UTF-8 characters.
        char mCommitHashShort[9];           // NUL-terminated first 8 hexadecimal commit-hash characters.
        uint8_t mCrc32[4];                  // CRC-32 of all preceding fields, stored little-endian.
    };

    bool ReadFlashPartition(Record& arRecord);
    bool SyncFlashPartitionWithFirmware(void);

    bool FindProductInfoInOtaImage(const esp_partition_t* apPartition, size_t aImageSize, Record& arRecord);
    bool IsNewVersionAllowed(const tSoftwareVersion& arNewVersion, tSoftwareVersion& arCurrentVersion);

    uint32_t CalculateCrc32(const uint8_t* apData, const size_t aLength);

} // namespace ProductInfoNS