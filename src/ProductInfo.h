#pragma once

#include <cstdint>

namespace ProductInfo
{
    /** Fixed-format record stored at flash offset 0x3FF000. */
    struct __attribute__((packed)) Record
    {
        uint8_t magic[8];
        uint8_t formatVersion;
        uint8_t softwareVersion[3];
        uint8_t commitHash[20];
        uint8_t commitCount[4];
        uint8_t workingTreeModified;
        uint8_t reserved[3];
        uint8_t crc32[4];
    };

    /**
     * @brief Read and validate product/build information from the read-only partition.
     *
     * The partition starts at flash address 0x3FF000 in productinfo_4mb.csv.
     *
     * @param[out] arRecord Receives the record after successful validation.
     * @return true if a valid record was read.
     */
    bool ReadFlashPartition(Record& arRecord);
}