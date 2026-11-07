/*
 * ProductInfo.cpp
 *
 *  Created on: 02.11.2026
 *      Author: hocki
 */
#include <Arduino.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <esp_partition.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ProductInfoRecord.h"   // Generated during the build; defines the ProductInfo record embedded in the firmware.
#include "ProductInfo.h"

namespace ProductInfoNS
{
/* Constants for validating the ProductInfo record */
constexpr uint8_t mcProductInfoMagicSize = PRODUCTINFO_MAGIC_SIZE;
constexpr const uint8_t mcProductInfoMagic[] = {PRODUCTINFO_MAGIC_BYTES};
constexpr uint8_t mcProductInfoFormatVersion = PRODUCTINFO_FORMAT_VERSION;

/* Chunk size for reading from the OTA image in bytes */
constexpr uint16_t mcReadChunkSize = 1024;

/* The partition subtype for the ProductInfo partition is defined as 0x40 in the custom partition table. */
constexpr uint8_t mcProductInfoPartitionSubtype = 0x40;

/* @brief Represents an invalid software version (255.255.255) */
constexpr tSoftwareVersion mcInvalidSoftwareVersion{0xFF, 0xFF, 0xFF};


/* Define offsets for for record layout checks */
#define PRODUCTINFO_SOFTWARE_VERSION_OFFSET     (PRODUCTINFO_MAGIC_SIZE              + PRODUCTINFO_FORMAT_VERSION_SIZE)
#define PRODUCTINFO_BUILD_DATETIME_OFFSET       (PRODUCTINFO_SOFTWARE_VERSION_OFFSET + PRODUCTINFO_SOFTWARE_VERSION_SIZE)
#define PRODUCTINFO_BRANCH_OFFSET               (PRODUCTINFO_BUILD_DATETIME_OFFSET   + PRODUCTINFO_BUILD_DATETIME_SIZE)
#define PRODUCTINFO_COMMIT_HASH_OFFSET          (PRODUCTINFO_BRANCH_OFFSET           + PRODUCTINFO_BRANCH_SIZE)

/* Define static assertion macros for record layout checks */
#define STATIC_ASSERT_RECORD_LAYOUT(condition, message) static_assert(condition, message)

#define STATIC_ASSERT_RECORD_OFFSET(field, expected_offset)                     \
    STATIC_ASSERT_RECORD_LAYOUT(offsetof(Record, field) == expected_offset,     \
    "Unexpected ProductInfo record offset for " #field)

/* Static assertions to ensure the Record structure has the expected layout and size */
STATIC_ASSERT_RECORD_OFFSET(mMagic,              0);
STATIC_ASSERT_RECORD_OFFSET(mFormatVersion,      PRODUCTINFO_MAGIC_SIZE);
STATIC_ASSERT_RECORD_OFFSET(mSoftwareVersion,    PRODUCTINFO_SOFTWARE_VERSION_OFFSET);
STATIC_ASSERT_RECORD_OFFSET(mBuildDateTimeLocal, PRODUCTINFO_BUILD_DATETIME_OFFSET);
STATIC_ASSERT_RECORD_OFFSET(mBranchShort,        PRODUCTINFO_BRANCH_OFFSET);
STATIC_ASSERT_RECORD_OFFSET(mCommitHashShort,    PRODUCTINFO_COMMIT_HASH_OFFSET);
STATIC_ASSERT_RECORD_OFFSET(mCrc32,              PRODUCTINFO_CRC32_OFFSET);

STATIC_ASSERT_RECORD_LAYOUT(sizeof(Record) == PRODUCTINFO_RECORD_SIZE,
    "Unexpected ProductInfo record size");
STATIC_ASSERT_RECORD_LAYOUT(sizeof(mcRecordData) == sizeof(Record),
    "Embedded ProductInfo must contain exactly one record");
// STATIC_ASSERT_RECORD_LAYOUT(
//     PRODUCTINFO_BRANCH_SIZE >= (PRODUCTINFO_SHORT_BRANCH_NAME_SIZE * 4 + 1),
//     "ProductInfo branch field cannot hold the configured UTF-8 branch name and NUL terminator");


/** @brief Get the product information record.
 *
 * @details
 * This function retrieves the product information record
 * that is embedded in the firmware image.
 *
 * @return The product information record.
*/
Record GetProductInfoRecord(void)
{
    /* Record object to hold the product information */
    Record wRecord;

    /* Retrieve the product information record from the embedded firmware image */
    std::memcpy(&wRecord, ProductInfoNS::mcRecordData, sizeof(Record));

    return wRecord; // Return the product information record
}

/**
 * @brief Find product information in an OTA image.
 *
 * @param apPartition Pointer to the OTA partition.
 * @param aImageSize Size of the OTA image.
 * @param arRecord Reference to the record structure to store the found product information.
 * @return true if the product information is found, false otherwise.
 */
bool FindProductInfoInOtaImage(const esp_partition_t* apPartition, size_t aImageSize, Record& arRecord)
{
    /* Flag indicating whether the product information has been found */
    bool wFound = false;
    /* Buffer for reading chunks from the OTA image */
    uint8_t wBuffer[mcReadChunkSize]{};
    uint8_t wChunksSinceYield = 0;

    for (size_t wOffset = 0; !wFound && wOffset < aImageSize;)
    {
        /* Determine the size of the next chunk to read */
        const size_t wReadSize = (aImageSize - wOffset < mcReadChunkSize)
            ? (aImageSize - wOffset) : mcReadChunkSize;

        /* Read a chunk of data from the OTA image partition into the buffer */
        if (esp_partition_read(apPartition, wOffset, wBuffer, wReadSize) != ESP_OK)
        {
            break;  // Error reading from the partition, exit the loop
        }

        /* Search within the current chunk */
        for (size_t wIndex = 0; (wIndex + mcProductInfoMagicSize) <= wReadSize; wIndex++)
        {
            /* Check for the magic bytes indicating the start of the product information */
            if (memcmp(wBuffer + wIndex, mcProductInfoMagic, mcProductInfoMagicSize) != 0)
            {
                continue; // Magic bytes not found, continue searching
            }

            /* Check if there is enough space for a full record and read it from the partition */
            const size_t wRecordOffset = wOffset + wIndex;
            if ((wRecordOffset + sizeof(arRecord)) > aImageSize ||
                esp_partition_read(apPartition, wRecordOffset, &arRecord, sizeof(arRecord)) != ESP_OK)
            {
                continue;   // Not enough space for a full record or error reading, continue searching
            }

            /* Calculate and verify the CRC32 of the record */
            const uint32_t wCalculatedCrc = CalculateCrc32(
                reinterpret_cast<const uint8_t*>(&arRecord), offsetof(Record, mCrc32));
            /* Retrieve the stored CRC from the record */
            const uint32_t wStoredCrc = static_cast<uint32_t>(arRecord.mCrc32[0]) |
                (static_cast<uint32_t>(arRecord.mCrc32[1]) << 8) |
                (static_cast<uint32_t>(arRecord.mCrc32[2]) << 16) |
                (static_cast<uint32_t>(arRecord.mCrc32[3]) << 24);

            /* Verify the format version and CRC32 */
            if ((arRecord.mFormatVersion == mcProductInfoFormatVersion) &&
                (wCalculatedCrc == wStoredCrc))
            {
                wFound = true;    // Valid product information found, return true
                break;  // Exit the inner loop once a valid record is found
            }
        }

        /* Check if there is enough data left to find a full record */
        if (wReadSize <= mcProductInfoMagicSize - 1)
        {
            break;  // Not enough data left to find a full record, exit the loop
        }

        /* Advance the offset for the next read chunk, ensuring overlap for magic bytes */
        wOffset += (wReadSize == mcReadChunkSize) ? (wReadSize - (mcProductInfoMagicSize - 1)) : wReadSize;

        /* OTA validation runs in AsyncTCP's watchdog-protected task. */
        esp_task_wdt_reset();
        if (++wChunksSinceYield >= 16)
        {
            wChunksSinceYield = 0;
            vTaskDelay(1);
        }
    }

    return wFound;  // Return whether product information was found
}

/** @brief Check if a new software version is allowed
 *
 * @param arNewVersion The new software version to check
 * @param arCurrentVersion The current software version
 * @return true if the new version is allowed, false otherwise
 */
bool IsNewVersionAllowed(const tSoftwareVersion& arNewVersion, tSoftwareVersion& arCurrentVersion)
{
    /* Flag indicating that the new version is allowed */
    bool wRetValue = false;     // default - not allowed

    /* Check if the new version is valid */
    if (arNewVersion.IsValid())
    {
        /* Retrieve the current product info record */
        Record wCurrentRecord = GetProductInfoRecord();

        /* Check if the current software version is valid */
        if (wCurrentRecord.mSoftwareVersion.IsValid())
        {
            /* If the current software version is valid, update the current version */
            arCurrentVersion = wCurrentRecord.mSoftwareVersion;
            /* and check if the new version is greater */
            wRetValue = (arNewVersion > wCurrentRecord.mSoftwareVersion);
        }
        else
        {
            /* If the current software version is invalid, set it to the default invalid version */
            arCurrentVersion = mcInvalidSoftwareVersion;
            wRetValue = true;
        }
    }

    return wRetValue;   // Return whether the new version is allowed
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