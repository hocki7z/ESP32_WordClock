#include "GitBuildInfo.h"

#include <Arduino.h>
#include <esp_partition.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "../AutoGitInfo.h"

namespace
{
constexpr uint8_t kMagic[8] = {'W', 'C', 'G', 'I', 'T', 'I', 'N', 'F'};
constexpr uint8_t kRecordFormatVersion = 1;
constexpr uint8_t kGitInfoPartitionSubtype = 0x40;

/*
 * Stable raw-flash record; all multibyte values are explicitly little-endian.
 * Offsets: magic 0..7, format 8, version 9..11, SHA-1 12..31,
 * commit count 32..35, modified flag 36, reserved 37..39, CRC-32 40..43.
 */
struct __attribute__((packed)) GitInfoRecord
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

static_assert(sizeof(GitInfoRecord) == 44, "Unexpected Git info record layout");
static_assert(offsetof(GitInfoRecord, crc32) == 40, "Unexpected Git info CRC offset");

uint32_t CalculateCrc32(const uint8_t* apData, const size_t aLength)
{
    uint32_t wCrc = 0xFFFFFFFF;
    for (size_t wIndex = 0; wIndex < aLength; ++wIndex)
    {
        wCrc ^= apData[wIndex];
        for (uint8_t wBit = 0; wBit < 8; ++wBit)
        {
            wCrc = (wCrc >> 1) ^ ((wCrc & 1U) ? 0xEDB88320U : 0U);
        }
    }
    return ~wCrc;
}

GitInfoRecord MakeRecord()
{
    GitInfoRecord wRecord{};
    memcpy(wRecord.magic, kMagic, sizeof(kMagic));
    wRecord.formatVersion = kRecordFormatVersion;
    wRecord.softwareVersion[0] = AUTO_GIT_INFO_SW_VERSION_MAJOR;
    wRecord.softwareVersion[1] = AUTO_GIT_INFO_SW_VERSION_MINOR;
    wRecord.softwareVersion[2] = AUTO_GIT_INFO_SW_VERSION_BUILD;

    constexpr uint8_t kCommitHash[20] = {AUTO_GIT_INFO_COMMIT_HASH_BYTES};
    memcpy(wRecord.commitHash, kCommitHash, sizeof(kCommitHash));

    constexpr uint32_t kCommitCount = AUTO_GIT_INFO_COMMITS_NUMBER;
    wRecord.commitCount[0] = static_cast<uint8_t>(kCommitCount);
    wRecord.commitCount[1] = static_cast<uint8_t>(kCommitCount >> 8);
    wRecord.commitCount[2] = static_cast<uint8_t>(kCommitCount >> 16);
    wRecord.commitCount[3] = static_cast<uint8_t>(kCommitCount >> 24);
    wRecord.workingTreeModified = AUTO_GIT_INFO_WORKING_TREE_MODIFIED;

    const uint32_t wCrc = CalculateCrc32(
        reinterpret_cast<const uint8_t*>(&wRecord), offsetof(GitInfoRecord, crc32));
    wRecord.crc32[0] = static_cast<uint8_t>(wCrc);
    wRecord.crc32[1] = static_cast<uint8_t>(wCrc >> 8);
    wRecord.crc32[2] = static_cast<uint8_t>(wCrc >> 16);
    wRecord.crc32[3] = static_cast<uint8_t>(wCrc >> 24);
    return wRecord;
}
}

namespace GitBuildInfo
{
bool UpdateFlashPartition()
{
    const esp_partition_t* const wpPartition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        static_cast<esp_partition_subtype_t>(kGitInfoPartitionSubtype),
        "gitinfo");
    if (wpPartition == nullptr || wpPartition->size < sizeof(GitInfoRecord))
    {
        Serial.println("GitBuildInfo: gitinfo partition not found or too small");
        return false;
    }

    const GitInfoRecord wExpected = MakeRecord();
    GitInfoRecord wCurrent{};
    if ((esp_partition_read(wpPartition, 0, &wCurrent, sizeof(wCurrent)) == ESP_OK) &&
        (memcmp(&wCurrent, &wExpected, sizeof(wExpected)) == 0))
    {
        return true;
    }

    esp_err_t wResult = esp_partition_erase_range(wpPartition, 0, wpPartition->size);
    if (wResult == ESP_OK)
    {
        wResult = esp_partition_write(wpPartition, 0, &wExpected, sizeof(wExpected));
    }

    if (wResult != ESP_OK)
    {
        Serial.printf("GitBuildInfo: partition update failed: %s\n", esp_err_to_name(wResult));
        return false;
    }

    Serial.println("GitBuildInfo: build metadata written to flash");
    return true;
}
}