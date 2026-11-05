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
#define PRODUCTINFO_FORMAT_VERSION  1

/* ProductInfo record field lengths in bytes; keep in sync with Record below. */
#define PRODUCTINFO_FORMAT_VERSION_SIZE     1
#define PRODUCTINFO_SOFTWARE_VERSION_SIZE   3
#define PRODUCTINFO_BUILD_DATETIME_SIZE     21
#define PRODUCTINFO_BRANCH_SIZE             81
#define PRODUCTINFO_COMMIT_HASH_SIZE        9
#define PRODUCTINFO_CRC32_SIZE              4

/* CRC covers all preceding fields; the record ends immediately after its CRC. */
#define PRODUCTINFO_CRC32_OFFSET            131
#define PRODUCTINFO_RECORD_SIZE             135

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
        /* Record identifier bytes */
        uint8_t mMagic[PRODUCTINFO_MAGIC_SIZE];
        /* Binary record layout version */
        uint8_t mFormatVersion;
        /* Software version of the firmware */
        tSoftwareVersion mSoftwareVersion;
        /*  NUL-terminated local build timestamp */
        char mBuildDateTimeLocal[PRODUCTINFO_BUILD_DATETIME_SIZE];
        /* NUL-terminated branch name, max 20 UTF-8 characters*/
        char mBranchShort[PRODUCTINFO_BRANCH_SIZE];
        /* NUL-terminated first 8 commit-hash characters*/
        char mCommitHashShort[PRODUCTINFO_COMMIT_HASH_SIZE];
        /* CRC-32 of preceding fields, stored little-endian*/
        uint8_t mCrc32[PRODUCTINFO_CRC32_SIZE];
    };

    Record GetProductInfoRecord(void);

    bool FindProductInfoInOtaImage(const esp_partition_t* apPartition, size_t aImageSize, Record& arRecord);
    bool IsNewVersionAllowed(const tSoftwareVersion& arNewVersion, tSoftwareVersion& arCurrentVersion);

    uint32_t CalculateCrc32(const uint8_t* apData, const size_t aLength);

} // namespace ProductInfoNS