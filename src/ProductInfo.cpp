/*
 * ProductInfo.cpp
 *
 *  Created on: 02.11.2026
 *      Author: hocki
 */
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <esp_partition.h>

#include "ProductInfo.h"


namespace
{
/* Constants for validating the ProductInfo record */
constexpr uint8_t mcProductInfoMagicSize = 8;
constexpr uint8_t mcProductInfoMagic[mcProductInfoMagicSize] = {'W', 'C', 'P', 'R', 'O', 'D', 'I', 'N'};
constexpr uint8_t mcProductInfoFormatVersion = 2;

/* Chunk size for reading from the OTA image in bytes */
constexpr uint16_t mcReadChunkSize = 1024;

/* The partition subtype for the ProductInfo partition is defined as 0x40 in the custom partition table. */
constexpr uint8_t mcProductInfoPartitionSubtype = 0x40;

/* Static assertions to ensure the Record structure has the expected layout and size */
static_assert(sizeof(ProductInfoNS::Record) == 131, "Unexpected product info record layout");
/* The CRC-32 field is expected to be at offset 127 in the Record structure */
static_assert(offsetof(ProductInfoNS::Record, mCrc32) == 127, "Unexpected product info CRC offset");
}

namespace ProductInfoNS
{
/* @brief Represents an invalid software version (255.255.255) */
constexpr tSoftwareVersion mcInvalidSoftwareVersion{0xFF, 0xFF, 0xFF};

/**
 * @brief Read and validate product/build information from the read-only partition.
 *
 * The partition starts at flash address 0x3FF000 in min_spiffs_custom.csv.
 * Date/time uses a NUL-terminated UTC ISO-8601 timestamp. The commit hash (8 hex chars)
 * and branch (up to 20 UTF-8 characters) are NUL-terminated strings.
 *
 * @param[out] arRecord Receives the record after successful validation.
 * @return true if a valid record was read.
 */
bool ReadFlashPartition(Record& arRecord)
{
    /* Initialize the return value to false, indicating failure by default */
    bool wRetValue = false;

    /* Find the first partition with the specified type, subtype, and label */
    const esp_partition_t* const wpPartition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        static_cast<esp_partition_subtype_t>(mcProductInfoPartitionSubtype),
        "productinfo");

    /* Check if the partition was found and has sufficient size for the record */
    if (wpPartition && (wpPartition->size >= sizeof(Record)))
    {
        Record wRecord{};

        /* Read the record from the partition into wRecord */
        if (esp_partition_read(wpPartition, 0, &wRecord, sizeof(wRecord)) == ESP_OK)
        {
            /* Validate the magic bytes and format version of the record */
            if ((memcmp(wRecord.mMagic, mcProductInfoMagic, sizeof(mcProductInfoMagic)) == 0) &&
                (wRecord.mFormatVersion == mcProductInfoFormatVersion))
            {
                /* Calculate the CRC-32 of the record (excluding the CRC field itself) */
                const uint32_t wCrc = CalculateCrc32(
                    reinterpret_cast<const uint8_t*>(&wRecord), offsetof(Record, mCrc32));
                /* Retrieve the stored CRC from the record */
                const uint32_t wStoredCrc =  static_cast<uint32_t>(wRecord.mCrc32[0])         |
                                            (static_cast<uint32_t>(wRecord.mCrc32[1]) << 8)   |
                                            (static_cast<uint32_t>(wRecord.mCrc32[2]) << 16)  |
                                            (static_cast<uint32_t>(wRecord.mCrc32[3]) << 24);

                /* Compare the calculated CRC with the stored CRC */
                if (wCrc == wStoredCrc)
                {
                    /* Assign the validated record to the output parameter */
                    arRecord = wRecord;
                    /* Set the return value to true, indicating success */
                    wRetValue = true;
                }
            }
        }
    }

    return wRetValue;
}

/**
 * @brief Calculate CRC-32 using the standard polynomial 0xEDB88320.
 *
 * @param apData Pointer to the data buffer.
 * @param aLength Length of the data buffer in bytes.
 * @return Calculated CRC-32 value.
 */
uint32_t CalculateCrc32(const uint8_t* apData, const size_t aLength)
{
    /* Initialize CRC to all 1s */
    uint32_t wCrc = 0xFFFFFFFF;

    /* Iterate over each byte in the data buffer */
    for (size_t wIndex = 0; wIndex < aLength; wIndex++)
    {
        /* XOR the current byte with the CRC */
        wCrc ^= apData[wIndex];

        /* Iterate over each bit in the current byte */
        for (uint8_t wBit = 0; wBit < 8; wBit++)
        {
            /* Shift one bit and XORs the reflected CRC-32 polynomial
             * when the outgoing low bit is set.
             *
             * 0xEDB88320U is the reflected bit-order form of
             * the standard CRC-32 polynomial 0x04C11DB7
             */
            wCrc = (wCrc >> 1) ^ ((wCrc & 1U) ? 0xEDB88320U : 0U);
        }
    }

    /* Return the final CRC value, inverted */
    return ~wCrc;
}

}   // namespace ProductInfoNS