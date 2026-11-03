/*
 * ProductInfo.h
 *
 *  Created on: 02.11.2026
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>

/**
 * Important:
 *   - This public record identifier provides neither encryption nor firmware authentication.
 *   - This definition is the single source of truth for the application and Python build tools.
 *   - Keep the identifier stable across releases. Devices expecting the previous identifier
 *     will reject OTA images containing a different one.
 */

/* The identifier is a 128-bit (16-byte) value, represented as 16 comma-separated hexadecimal bytes. */
#define PRODUCTINFO_MAGIC_BYTES     0x59U, 0x7AU, 0x96U, 0x32U, 0x9FU, 0xA4U, 0x0DU, 0xEEU, 0xE9U, 0x8DU, 0x23U, 0x37U, 0x52U, 0x91U, 0xEBU, 0xF2U
#define PRODUCTINFO_MAGIC_SIZE      16

/* The format version is a single byte (0-255) that indicates the layout of the ProductInfo record. */
#define PRODUCTINFO_FORMAT_VERSION  3

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
        uint8_t mMagic[PRODUCTINFO_MAGIC_SIZE];     // Record identifier bytes.
        uint8_t mFormatVersion;                     // Binary record layout version.
        tSoftwareVersion mSoftwareVersion;          // Software version of the firmware.
        char mBuildDateTimeUtc[25];                 // NUL-terminated UTC ISO-8601 build timestamp.
        char mBranchShort[81];                      // NUL-terminated branch name, limited to 20 UTF-8 characters.
        char mCommitHashShort[9];                   // NUL-terminated first 8 hexadecimal commit-hash characters.
        uint8_t mCrc32[4];                          // CRC-32 of all preceding fields, stored little-endian.
    };

    bool ReadFlashPartition(Record& arRecord);
    bool SyncFlashPartitionWithFirmware(void);

    bool FindProductInfoInOtaImage(const esp_partition_t* apPartition, size_t aImageSize, Record& arRecord);
    bool IsNewVersionAllowed(const tSoftwareVersion& arNewVersion, tSoftwareVersion& arCurrentVersion);

    uint32_t CalculateCrc32(const uint8_t* apData, const size_t aLength);

} // namespace ProductInfoNS