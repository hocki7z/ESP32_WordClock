#include "ProductInfo.h"

#include <esp_partition.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace
{
constexpr uint8_t kMagic[8] = {'W', 'C', 'P', 'R', 'O', 'D', 'I', 'N'};
constexpr uint8_t kRecordFormatVersion = 2;
constexpr uint8_t kProductInfoPartitionSubtype = 0x40;
static_assert(sizeof(ProductInfo::Record) == 131, "Unexpected product info record layout");
static_assert(offsetof(ProductInfo::Record, crc32) == 127, "Unexpected product info CRC offset");

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
}

namespace ProductInfo
{
bool ReadFlashPartition(Record& arRecord)
{
    const esp_partition_t* const wpPartition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        static_cast<esp_partition_subtype_t>(kProductInfoPartitionSubtype),
        "productinfo");
    if (wpPartition == nullptr || wpPartition->size < sizeof(Record))
    {
        return false;
    }

    Record wRecord{};
    if (esp_partition_read(wpPartition, 0, &wRecord, sizeof(wRecord)) != ESP_OK)
    {
        return false;
    }

    if (memcmp(wRecord.magic, kMagic, sizeof(kMagic)) != 0 ||
        wRecord.formatVersion != kRecordFormatVersion)
    {
        return false;
    }

    const uint32_t wCrc = CalculateCrc32(
        reinterpret_cast<const uint8_t*>(&wRecord), offsetof(Record, crc32));
    const uint32_t wStoredCrc = static_cast<uint32_t>(wRecord.crc32[0]) |
                                (static_cast<uint32_t>(wRecord.crc32[1]) << 8) |
                                (static_cast<uint32_t>(wRecord.crc32[2]) << 16) |
                                (static_cast<uint32_t>(wRecord.crc32[3]) << 24);
    if (wCrc != wStoredCrc)
    {
        return false;
    }

    arRecord = wRecord;
    return true;
}
}