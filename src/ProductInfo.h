#pragma once

#include <Arduino.h>

#include <cstdint>

#include <esp_ota_ops.h>
#include <esp_system.h>

namespace ProductInfoNS
{
    struct tSoftwareVersion
    {
        uint8_t mMajor;  // Software version major component.
        uint8_t mMinor;  // Software version minor component.
        uint8_t mBuild;  // Software version build/patch component.

        bool operator==(const tSoftwareVersion& other) const
        {
            return mMajor == other.mMajor && mMinor == other.mMinor && mBuild == other.mBuild;
        }

        bool operator!=(const tSoftwareVersion& other) const
        {
            return !(*this == other);
        }

        bool operator<(const tSoftwareVersion& other) const
        {
            if (mMajor != other.mMajor)
                return mMajor < other.mMajor;
            if (mMinor != other.mMinor)
                return mMinor < other.mMinor;
            return mBuild < other.mBuild;
        }

        bool operator>(const tSoftwareVersion& other) const
        {
            // Check if != and not <
            return *this != other && !(*this < other);
        }

        bool IsValid(void) const
        {
            return mMajor != 0xFF || mMinor != 0xFF || mBuild != 0xFF;
        }
    };

    /** Fixed-format record stored at flash offset 0x3FF000. */
    struct __attribute__((packed)) Record
    {
        uint8_t magic[8];                  // Record identifier bytes: "WCPRODIN".
        uint8_t formatVersion;             // Binary record layout version.
        tSoftwareVersion softwareVersion; // Major, minor, and build version bytes.
        char buildDateTimeUtc[25];         // NUL-terminated UTC ISO-8601 build timestamp.
        char branchShort[81];              // NUL-terminated branch name, limited to 20 UTF-8 characters.
        char commitHashShort[9];           // NUL-terminated first 8 hexadecimal commit-hash characters.
        uint8_t crc32[4];                  // CRC-32 of all preceding fields, stored little-endian.
    };

    /**
     * @brief Read and validate product/build information from the read-only partition.
     *
     * The partition starts at flash address 0x3FF000 in productinfo_4mb.csv.
      * Date/time uses a NUL-terminated UTC ISO-8601 timestamp. The commit hash (8 hex chars)
      * and branch (up to 20 UTF-8 characters) are NUL-terminated strings.
     *
     * @param[out] arRecord Receives the record after successful validation.
     * @return true if a valid record was read.
     */
    bool ReadFlashPartition(Record& arRecord);

    /**
     * @brief Restore the ProductInfo partition from the copy embedded in this firmware.
     *
     * Call early during startup. This lets a standard application OTA image carry
     * both the application and its matching ProductInfo data.
     *
     * @return true if the partition matches the embedded image and contains a valid record.
     */
    bool SyncFlashPartitionWithFirmware();

    uint32_t CalculateCrc32(const uint8_t* apData, const size_t aLength);
    bool IsInvalidSoftwareVersion(const tSoftwareVersion& arVersion);

    bool FindProductInfoInOtaImage(const esp_partition_t* apPartition, size_t aImageSize, Record& arRecord);
    int CompareSoftwareVersions(const tSoftwareVersion& arLeft, const tSoftwareVersion& arRight);
    bool IsNewVersionAllowed(const tSoftwareVersion& arNewVersion, tSoftwareVersion& arCurrentVersion);
} // namespace ProductInfoNS