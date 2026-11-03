/*
 * OTAUpdate.cpp
 *
 *  Created on: 02.11.2026
 *      Author: hocki
 */
#include <ESPAsyncWebServer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>

#include <cstddef>
#include <cstdint>

#include "ProductInfo.h"
#include "OTAUpdate.h"
#include "OTAUpdateCustomJs.h"

namespace
{
enum class OtaUploadStatus : uint8_t
{
    Idle,
    Receiving,
    Failed,
    ReadyToBoot,
};

AsyncWebServer* mpOtaServer = nullptr;
AsyncWebServerRequest* mpOtaRequest = nullptr;
esp_ota_handle_t mOtaHandle = 0;
bool mOtaHandleValid = false;
const esp_partition_t* mpOtaPartition = nullptr;
size_t mOtaBytesReceived = 0;
OtaUploadStatus mOtaStatus = OtaUploadStatus::Idle;
esp_err_t mOtaError = ESP_OK;
String mOtaFailureReason;

void AbortOtaUpload()
{
    if (mOtaHandleValid)
    {
        esp_ota_abort(mOtaHandle);
        mOtaHandle = 0;
        mOtaHandleValid = false;
    }
}

void RestartAfterOta(void*)
{
    vTaskDelay(pdMS_TO_TICKS(1500));
    ESP.restart();
    vTaskDelete(nullptr);
}
}

namespace OTAUpdateNS
{
const char* CustomJavaScript()
{
    return OTAUpdateAssets::kCustomJavaScript;
}

void RegisterRoutes(AsyncWebServer* apServer)
{
    if (apServer == nullptr || apServer == mpOtaServer)
    {
        return;
    }

    apServer->on("/update", HTTP_POST,
        [](AsyncWebServerRequest* apRequest)
        {
            if (mpOtaRequest != apRequest)
            {
                apRequest->send(400, "text/plain", "No firmware image was received or another upload is active.");
                return;
            }

            const OtaUploadStatus wStatus = mOtaStatus;
            const esp_err_t wError = mOtaError;
            const String wFailureReason = mOtaFailureReason;
            mpOtaRequest = nullptr;
            mOtaStatus = OtaUploadStatus::Idle;
            mpOtaPartition = nullptr;
            mOtaBytesReceived = 0;
            mOtaFailureReason = "";

            if (wStatus == OtaUploadStatus::ReadyToBoot)
            {
                apRequest->send(200, "text/plain", "Firmware accepted. The device is restarting now.");
                xTaskCreate(RestartAfterOta, "ota_restart", 2048, nullptr, 1, nullptr);
                return;
            }

            apRequest->send(400, "text/plain",
                wFailureReason.isEmpty()
                    ? String("Firmware update failed (ESP error ") + String(static_cast<int>(wError)) + ")."
                    : wFailureReason);
        },
        [](AsyncWebServerRequest* apRequest, const String& arFilename, size_t aIndex,
            uint8_t* apData, size_t aLength, bool aFinal)
        {
            if (aIndex == 0)
            {
                if (mpOtaRequest != nullptr && mpOtaRequest != apRequest)
                {
                    return;
                }

                mpOtaRequest = apRequest;
                mOtaStatus = OtaUploadStatus::Failed;
                mOtaError = ESP_FAIL;
                mOtaFailureReason = "Invalid firmware upload.";

                String wLowerFilename = arFilename;
                wLowerFilename.toLowerCase();
                if (!wLowerFilename.endsWith(".bin"))
                {
                    return;
                }

                mpOtaPartition = esp_ota_get_next_update_partition(nullptr);
                if (mpOtaPartition == nullptr)
                {
                    mOtaError = ESP_ERR_NOT_FOUND;
                    return;
                }

                mOtaError = esp_ota_begin(mpOtaPartition, OTA_SIZE_UNKNOWN, &mOtaHandle);
                if (mOtaError != ESP_OK)
                {
                    mOtaHandle = 0;
                    mOtaHandleValid = false;
                    return;
                }

                mOtaHandleValid = true;
                mOtaBytesReceived = 0;
                mOtaStatus = OtaUploadStatus::Receiving;
            }

            if (mpOtaRequest != apRequest || mOtaStatus != OtaUploadStatus::Receiving)
            {
                return;
            }

            if (aLength > mpOtaPartition->size - mOtaBytesReceived)
            {
                AbortOtaUpload();
                mOtaStatus = OtaUploadStatus::Failed;
                mOtaError = ESP_ERR_INVALID_SIZE;
                return;
            }

            mOtaError = esp_ota_write(mOtaHandle, apData, aLength);
            if (mOtaError != ESP_OK)
            {
                AbortOtaUpload();
                mOtaStatus = OtaUploadStatus::Failed;
                return;
            }
            mOtaBytesReceived += aLength;

            if (aFinal)
            {
                mOtaError = esp_ota_end(mOtaHandle);
                mOtaHandle = 0;
                mOtaHandleValid = false;
                if (mOtaError != ESP_OK)
                {
                    mOtaFailureReason = "The uploaded file is not a valid ESP32 application image.";
                    mOtaStatus = OtaUploadStatus::Failed;
                    return;
                }

                ProductInfoNS::Record wNewRecord{};
                if (!ProductInfoNS::FindProductInfoInOtaImage(mpOtaPartition, mOtaBytesReceived, wNewRecord))
                {
                    mOtaError = ESP_ERR_INVALID_CRC;
                    mOtaFailureReason = "No valid embedded ProductInfo record was found in the firmware image.";
                    mOtaStatus = OtaUploadStatus::Failed;
                    return;
                }

                ProductInfoNS::tSoftwareVersion wCurrentVersion{};
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

                mOtaError = esp_ota_set_boot_partition(mpOtaPartition);
                mOtaStatus = (mOtaError == ESP_OK)
                    ? OtaUploadStatus::ReadyToBoot
                    : OtaUploadStatus::Failed;
            }
        });

    mpOtaServer = apServer;
}
}
