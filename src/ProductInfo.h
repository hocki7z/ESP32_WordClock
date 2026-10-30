#pragma once

#include <cstdint>

namespace ProductInfoNS
{
    struct tSoftwareVersion
    {
        uint8_t mMajor;  // Software version major component.
        uint8_t mMinor;  // Software version minor component.
        uint8_t mBuild;  // Software version build/patch component.
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
}