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

#include "ProductInfoImage.h"   // Generated during the build; defines the ProductInfo record embedded in the firmware.
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


/* Static assertions to ensure the Record structure has the expected layout and size */
static_assert(sizeof(ProductInfoNS::Record) == 139, "Unexpected product info record layout");
/* The CRC-32 field is expected to be at offset 135 in the Record structure */
static_assert(offsetof(ProductInfoNS::Record, mCrc32) == 135, "Unexpected product info CRC offset");
/* The embedded product info in the firmware image must contain exactly one record */
static_assert(sizeof(ProductInfoNS::ProductInfoImage::mcData) == sizeof(ProductInfoNS::Record),
    "Embedded product info must contain exactly one record");


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
            if ((memcmp(wRecord.mMagic, mcProductInfoMagic, mcProductInfoMagicSize) == 0) &&
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
 * @brief Synchronize the flash partition with the firmware image.
 *
 * This function checks if the product information in the flash partition matches
 * the product information embedded in the firmware image. If they do not match,
 * it erases the flash partition and writes the firmware's product information to it.
 *
 * @return true if the synchronization was successful or unnecessary, false otherwise.
 */
bool SyncFlashPartitionWithFirmware(void)
{
    /* Flag indicating whether the synchronization succeeded */
    bool wSyncSucceeded = false;    // default - not succeeded

    /* Find the flash partition for product information */
    const esp_partition_t* const wpPartition = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        static_cast<esp_partition_subtype_t>(mcProductInfoPartitionSubtype),
        "productinfo");

    /* Check if the partition was found and is large enough to hold the record */
    if ((wpPartition != nullptr) &&
        (wpPartition->size >= sizeof(ProductInfoImage::mcData)))
    {
        constexpr size_t kCompareChunkSize = 128;
        uint8_t wBuffer[kCompareChunkSize]{};

        bool wRecordMatches = true;
        bool wReadSucceeded = true;


        for (size_t wOffset = 0; wOffset < sizeof(ProductInfoImage::mcData); wOffset += sizeof(wBuffer))
        {
            /* Determine the size of the current chunk to read */
            const size_t wChunkSize = sizeof(ProductInfoImage::mcData) - wOffset < sizeof(wBuffer)
                                          ? (sizeof(ProductInfoImage::mcData) - wOffset) : sizeof(wBuffer);

            /* Read the current chunk from the flash partition */
            if (esp_partition_read(wpPartition, wOffset, wBuffer, wChunkSize) != ESP_OK)
            {
                wReadSucceeded = false;     // Error reading from the partition, exit the loop
                break;
            }

            /* Compare the current chunk with the corresponding chunk in the firmware image */
            if (memcmp(wBuffer, ProductInfoImage::mcData + wOffset, wChunkSize) != 0)
            {
                wRecordMatches = false;   // The records do not match, exit the loop
                break;
            }
        }

        /* If the read succeeded, check if the images match */
        if (wReadSucceeded)
        {
            wSyncSucceeded = true;

            /* If the record does not match, erase the full partition and write the record */
            if (!wRecordMatches)
            {
                wSyncSucceeded = (esp_partition_erase_range(wpPartition, 0, wpPartition->size) == ESP_OK) &&
                    (esp_partition_write(wpPartition, 0, ProductInfoImage::mcData, sizeof(ProductInfoImage::mcData)) == ESP_OK);
            }

            /* If the synchronization succeeded, read back the record to verify */
            if (wSyncSucceeded)
            {
                Record wRecord{};
                wSyncSucceeded = ReadFlashPartition(wRecord);
            }
        }
    }

    return wSyncSucceeded;
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
        Record wCurrentRecord{};

        /* Read the current software version from the flash partition */
        if (!ReadFlashPartition(wCurrentRecord) ||
            !wCurrentRecord.mSoftwareVersion.IsValid())
        {
            /* If the current software version is invalid, set it to the default invalid version */
            arCurrentVersion = mcInvalidSoftwareVersion;
            wRetValue = true;
        }
        else
        {
            /* If the current software version is valid, update the current version */
            arCurrentVersion = wCurrentRecord.mSoftwareVersion;
            /* and check if the new version is greater */
            wRetValue = (arNewVersion > wCurrentRecord.mSoftwareVersion);
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