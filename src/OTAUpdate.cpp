/*
 * OTAUpdate.cpp
 *
 *  Created on: 02.11.2026
 *      Author: hocki
 */
#include <Arduino.h>

#include <cstddef>
#include <cstdint>
#include <atomic>

#include <ESPAsyncWebServer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>


#include "Logger.h"
#include "Configuration.h"
#include "ProductInfo.h"

#include "OTAUpdateCustomJs.h"      // Generated during the build
#include "OTAUpdate.h"


/* Log level for this module */
#define LOG_LEVEL_OTA_UPDATE_NS    (LOG_DEBUG)

namespace OTAUpdateNS
{
/** @brief Enum representing the status of an OTA upload. */
enum class OtaUploadStatus : uint8_t
{
    Idle,           ///< No OTA upload is in progress
    Receiving,      ///< OTA upload is currently being received
//    Validating,     ///< OTA upload is being validated
    Failed,         ///< OTA upload has failed
//    Success,        ///< OTA upload has completed successfully
    ReadyToBoot,    ///< OTA upload is ready to boot
};

/** @brief Pointer to the AsyncWebServer instance handling OTA updates */
static AsyncWebServer* mpOtaServer = nullptr;
/** @brief Pointer to the current OTA upload request */
static AsyncWebServerRequest* mpOtaRequest = nullptr;

/** @brief Handle for the current OTA update */
static esp_ota_handle_t mOtaHandle = 0;
/** @brief Flag indicating whether the OTA handle is valid */
static bool mOtaHandleValid = false;

/** @brief Pointer to the OTA partition being written to */
static const esp_partition_t* mpOtaPartition = nullptr;
/** @brief Number of OTA bytes received */
static size_t mOtaBytesReceived = 0;

/** @brief Current OTA upload status */
static OtaUploadStatus mOtaStatus = OtaUploadStatus::Idle;
/** @brief Last OTA error code */
static esp_err_t mOtaError = ESP_OK;
/** @brief Reason for OTA failure */
static String mOtaFailureReason;

/** @brief Flag indicating whether an OTA upload is active */
static std::atomic<bool> mOtaUploadActive{false};


/** @brief Abort the ongoing OTA upload.
 *
 * @details
 * This function aborts the current OTA upload if it is in progress.
 * It resets the OTA handle and marks it as invalid.
 */
static void AbortOtaUpload(void)
{
    if (mOtaHandleValid)
    {
        /* Abort the OTA update and reset the handle */
        esp_ota_abort(mOtaHandle);
        mOtaHandle = 0;

        mOtaHandleValid = false;
    }
}

/** @brief Restart the system after a short delay following an OTA update.
 *
 * @details
 * This function delays for 1.5 seconds to allow any pending operations to complete,
 * then restarts the ESP32. It is intended to be used as a task function.
 */
static void RestartAfterOta(void*)
{
    vTaskDelay(pdMS_TO_TICKS(1500));
    ESP.restart();
    vTaskDelete(nullptr);
}

/** @brief Handle an OTA upload request.
 *
 * @details
 * This function is called when an OTA upload request is received.
 * It checks the status of the upload, and if the upload is complete,
 * it sends an appropriate response to the client.
 *
 * If the upload was successful, it schedules a system restart.
 *
 * @param apRequest Pointer to the AsyncWebServerRequest object representing the OTA request.
 */
static void HandleOtaRequest(AsyncWebServerRequest* apRequest)
{
    /* Check if the incoming request is the current OTA request */
    if (apRequest == mpOtaRequest)
    {
        /* Store the current OTA status, error, and failure reason */
        const OtaUploadStatus wStatus = mOtaStatus;
        const esp_err_t wError = mOtaError;
        const String wFailureReason = mOtaFailureReason;

        /* Mark the OTA upload as inactive and reset relevant variables */
        mOtaUploadActive.store(false);
        mpOtaRequest = nullptr;
        mOtaStatus = OtaUploadStatus::Idle;
        mpOtaPartition = nullptr;
        mOtaBytesReceived = 0;
        mOtaFailureReason = "";

        if (wStatus == OtaUploadStatus::ReadyToBoot)
        {
            /* If the OTA upload was successful, send a success response */
            apRequest->send(200, "text/plain", "Firmware accepted. The device is restarting now.");
            /* and create a task to restart the device after a short delay */
            xTaskCreate(RestartAfterOta, "ota_restart", 1024, nullptr, 1, nullptr);
            return;
        }

        /* OTA upload failed */
        LOG_WITH_REF(LOG_ERROR, LOG_LEVEL_OTA_UPDATE_NS, "OTA upload failed: status %d, error %d, reason: %s",
            static_cast<int>(wStatus), static_cast<int>(wError), wFailureReason.c_str());

        /* Send a failure response */
        apRequest->send(400, "text/plain",
            wFailureReason.isEmpty()
                ? String("Firmware update failed (ESP error ") + String(static_cast<int>(wError)) + ")."
                : wFailureReason);
    }
    else
    {
        /* If the request is not the current OTA request, send a 400 Bad Request response */
        apRequest->send(400, "text/plain", "No firmware image was received or another upload is active.");
    }
}

/**
 * @brief Handle OTA upload requests.
 *
 * @details
 * This function is called when an OTA upload request is received.
 * It manages the OTA upload process, including initialization, validation,
 * and handling of incoming firmware data chunks.
 *
 * @param apRequest Pointer to the AsyncWebServerRequest object.
 * @param arFilename Name of the uploaded firmware file.
 * @param aIndex Index of the current data chunk.
 * @param apData Pointer to the data chunk.
 * @param aLength Length of the data chunk.
 * @param aFinal Indicates if this is the final data chunk.
 */
static void HandleOtaUpload(AsyncWebServerRequest* apRequest, const String& arFilename, size_t aIndex,
    uint8_t* apData, size_t aLength, bool aFinal)
{
    /* Handle the incoming OTA firmware data in chunks */

    /* If this is the first chunk of data (index 0), initialize the OTA process */
    if (aIndex == 0)
    {
        // Check if another OTA upload is already in progress
        if (mpOtaRequest != nullptr && mpOtaRequest != apRequest)
        {
            return;
        }

        /* Store the current OTA request pointer */
        mpOtaRequest = apRequest;

        /* Set up a disconnect handler for the OTA request */
        apRequest->onDisconnect([apRequest]()
        {
            if (mpOtaRequest == apRequest)
            {
                AbortOtaUpload();
                mpOtaRequest = nullptr;
                mOtaStatus = OtaUploadStatus::Failed;
                mOtaFailureReason = "OTA upload disconnected.";
                mOtaUploadActive.store(false);
            }
        });

        /* Mark the OTA upload as failed initially */
        mOtaStatus = OtaUploadStatus::Failed;
        mOtaError = ESP_FAIL;
        mOtaFailureReason = "Invalid firmware upload.";

        /* Validate that the uploaded file has a ".bin" extension */
        String wLowerFilename = arFilename;
        wLowerFilename.toLowerCase();
        if (!wLowerFilename.endsWith(".bin"))
        {
            return;
        }

        /* Check if an OTA update partition is available */
        mpOtaPartition = esp_ota_get_next_update_partition(nullptr);
        if (mpOtaPartition == nullptr)
        {
            mOtaError = ESP_ERR_NOT_FOUND;
            return;
        }

        /* If the OTA update could not be started, reset the handle and mark it as invalid */
        mOtaError = esp_ota_begin(mpOtaPartition, OTA_SIZE_UNKNOWN, &mOtaHandle);
        if (mOtaError != ESP_OK)
        {
            mOtaHandle = 0;
            mOtaHandleValid = false;
            return;
        }

        /* Mark the OTA update as started */
        mOtaHandleValid = true;
        mOtaBytesReceived = 0;
        mOtaStatus = OtaUploadStatus::Receiving;
        mOtaUploadActive.store(true);
    }

    /* Ignore data for another request or an upload that is not receiving */
    if (mpOtaRequest != apRequest || mOtaStatus != OtaUploadStatus::Receiving)
    {
        return;
    }

    /* Abort the OTA upload if the data exceeds the partition size */
    if (aLength > mpOtaPartition->size - mOtaBytesReceived)
    {
        AbortOtaUpload();
        mOtaStatus = OtaUploadStatus::Failed;
        mOtaError = ESP_ERR_INVALID_SIZE;
        return;
    }

    /* Abort if the received data index does not match the expected offset */
    if (aIndex != mOtaBytesReceived)
    {
        AbortOtaUpload();
        mOtaStatus = OtaUploadStatus::Failed;
        mOtaError = ESP_ERR_INVALID_SIZE;
        mOtaFailureReason = "Firmware upload data was incomplete or out of order.";

        LOG_WITH_REF(LOG_ERROR, LOG_LEVEL_OTA_UPDATE_NS,
            "OTA upload offset mismatch: received offset %lu, expected %lu",
            static_cast<unsigned long>(aIndex),
            static_cast<unsigned long>(mOtaBytesReceived));
        return;
    }

    /* Abort the OTA upload if writing to the partition fails */
    mOtaError = esp_ota_write(mOtaHandle, apData, aLength);
    if (mOtaError != ESP_OK)
    {
        AbortOtaUpload();
        mOtaStatus = OtaUploadStatus::Failed;
        return;
    }

    /* Update the number of bytes received */
    mOtaBytesReceived += aLength;

    /* Finalize and validate the OTA image after the final chunk */
    if (aFinal)
    {
        mOtaError = esp_ota_end(mOtaHandle);
        mOtaHandle = 0;
        mOtaHandleValid = false;

        if (mOtaError != ESP_OK)
        {
            mOtaStatus = OtaUploadStatus::Failed;
            mOtaFailureReason = "The uploaded file is not a valid ESP32 application image.";
            LOG_WITH_REF(LOG_ERROR, LOG_LEVEL_OTA_UPDATE_NS,
                "esp_ota_end failed: error %d, received %lu bytes",
                static_cast<int>(mOtaError),
                static_cast<unsigned long>(mOtaBytesReceived));
            return;
        }

        /* Validate the OTA image by checking for a valid embedded ProductInfo record */
        ProductInfoNS::Record wNewRecord{};
        ProductInfoNS::tSoftwareVersion wCurrentVersion{};

        if (!ProductInfoNS::FindProductInfoInOtaImage(mpOtaPartition, mOtaBytesReceived, wNewRecord))
        {
            mOtaError = ESP_ERR_INVALID_CRC;
            mOtaFailureReason = "No valid embedded ProductInfo record was found in the firmware image.";
            mOtaStatus = OtaUploadStatus::Failed;
            return;
        }

        if (!wNewRecord.mSoftwareVersion.IsValid())
        {
            mOtaError = ESP_FAIL;
            mOtaFailureReason = "Update refused: the selected firmware version is invalid (255.255.255).";
            mOtaStatus = OtaUploadStatus::Failed;
            return;
        }

        if (!ProductInfoNS::IsNewVersionAllowed(wNewRecord.mSoftwareVersion, wCurrentVersion))
        {
            char wMessage[128]{};
            snprintf(wMessage, sizeof(wMessage),
                "Update refused: firmware version %u.%u.%u is not newer than installed version %u.%u.%u.",
                static_cast<unsigned>(wNewRecord.mSoftwareVersion.mMajor),
                static_cast<unsigned>(wNewRecord.mSoftwareVersion.mMinor),
                static_cast<unsigned>(wNewRecord.mSoftwareVersion.mBuild),
                static_cast<unsigned>(wCurrentVersion.mMajor),
                static_cast<unsigned>(wCurrentVersion.mMinor),
                static_cast<unsigned>(wCurrentVersion.mBuild));

            mOtaFailureReason = wMessage;
            mOtaError = ESP_FAIL;
            mOtaStatus = OtaUploadStatus::Failed;
            return;
        }

        /* Set the boot partition to the newly uploaded OTA partition */
        mOtaError = esp_ota_set_boot_partition(mpOtaPartition);

        /* Update the pending firmware update flag in the settings */
        Settings.SetValue<bool>(ConfigNS::mKeyPendingFirmwareUpdate, (mOtaError == ESP_OK));

        mOtaStatus = (mOtaError == ESP_OK)
            ? OtaUploadStatus::ReadyToBoot
            : OtaUploadStatus::Failed;
    }
}

/** @brief Return the custom JavaScript injected into the ESPUI page.
 *
 * @details
 * This function returns a pointer to the custom JavaScript code
 * that is injected into the ESPUI page for OTA update functionality.
 * The JavaScript code is generated during the build process and is
 * stored in the OTAUpdateCustomJs.h header file.
 *
 * @return A pointer to the custom JavaScript code as a null-terminated string.
*/
const char* CustomJavaScript(void)
{
    return OTAUpdateAssets::kCustomJavaScript;  // Return the generated JavaScript code
}

/** @brief Return whether an OTA upload is currently being received or validated.
 *
 * @details
 * This function checks if an OTA upload is currently in progress, either being received or validated.
 * It returns true if an OTA upload is active, and false otherwise.
 *
 * @return true if an OTA upload is active, false otherwise.
*/
bool IsUploadActive(void)
{
    return mOtaUploadActive.load();
}

/** @brief Register OTA endpoints on the web server already started by ESPUI.
 *
 * @details
 * This function registers the necessary endpoints on the provided AsyncWebServer
 * instance for handling OTA updates. It sets up the "/update" endpoint to handle
 * OTA upload requests, including receiving firmware data, validating it, and managing
 * the OTA process.
 * The function also handles disconnections and errors during the OTA process, ensuring
 * that the system remains stable and responsive.
 *
 * @param apServer Pointer to the AsyncWebServer instance where OTA endpoints will be registered.
 * @note This function should be called after the web server has been started by ESPUI.
*/
void RegisterRoutes(AsyncWebServer* apServer)
{
    if ((apServer != nullptr) && (apServer != mpOtaServer))
    {
        mpOtaServer = apServer;
        mpOtaServer->on("/update", HTTP_POST, HandleOtaRequest, HandleOtaUpload);
    }
}

}   // namespace OTAUpdateNS
