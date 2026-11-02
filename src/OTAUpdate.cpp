#include "OTAUpdate.h"

#include <ESPAsyncWebServer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>

#include <cstddef>
#include <cstdint>

#include "ProductInfo.h"

namespace
{
constexpr char kOtaPage[] = R"html(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta http-equiv="refresh" content="0;url=/#settings"><title>Firmware update</title></head>
<body><p>Opening the Settings firmware uploader… <a href="/#settings">Continue</a></p></body></html>)html";

const char kOtaCustomJs[] = R"js((() => {
    function addFirmwareUploader() {
        const settings = document.getElementById('settings');
        if (!settings) return;

        const card = Array.from(settings.children).find(element => {
            if (!element.classList.contains('card')) return false;
            const heading = element.querySelector('h5');
            return heading && heading.textContent.trim() === 'Firmware Update';
        });
        if (!card || document.getElementById('wordclock-ota-uploader')) return;

        const panel = document.createElement('div');
        panel.id = 'wordclock-ota-uploader';
        panel.style.cssText = 'padding:12px 0;text-align:left';

        const description = document.createElement('p');
        description.textContent = 'Select the versioned <project>_ota_<major>_<minor>_<build>.bin file from your PlatformIO build output.';
        const picker = document.createElement('input');
        picker.type = 'file';
        picker.accept = '.bin,application/octet-stream';
        picker.style.cssText = 'display:block;width:100%;margin:8px 0';

        const button = document.createElement('button');
        button.type = 'button';
        button.textContent = 'Upload and install';
        button.style.cssText = 'width:100%;padding:10px;cursor:pointer';

        const progress = document.createElement('progress');
        progress.max = 100;
        progress.value = 0;
        progress.hidden = true;
        progress.style.cssText = 'display:block;width:100%;margin:10px 0';

        const status = document.createElement('p');
        status.setAttribute('role', 'status');
        status.textContent = 'Choose a .bin image to begin.';

        button.addEventListener('click', () => {
            const file = picker.files && picker.files[0];
            if (!file || !file.name.toLowerCase().endsWith('.bin')) {
                status.textContent = 'Select a .bin application image.';
                return;
            }

            button.disabled = true;
            picker.disabled = true;
            status.textContent = 'Starting firmware upload…';
            progress.hidden = false;
            progress.value = 0;

            const form = new FormData();
            form.append('firmware', file, file.name);
            const request = new XMLHttpRequest();
            request.open('POST', '/update');
            request.upload.onprogress = event => {
                if (event.lengthComputable) {
                    progress.value = Math.round(event.loaded * 100 / event.total);
                    status.textContent = 'Uploading ' + progress.value + '%';
                }
            };
            request.onload = () => {
                status.textContent = request.responseText || ('Update failed (' + request.status + ').');
                if (request.status !== 200) {
                    button.disabled = false;
                    picker.disabled = false;
                }
            };
            request.onerror = () => {
                status.textContent = 'Connection lost. The device may be restarting; wait and reconnect.';
                button.disabled = false;
                picker.disabled = false;
            };
            request.send(form);
        });

        panel.append(description, picker, button, progress, status);
        card.appendChild(panel);
    }

    const observer = new MutationObserver(addFirmwareUploader);
    observer.observe(document.documentElement, { childList: true, subtree: true });
    addFirmwareUploader();
})();)js";

enum class OtaUploadStatus : uint8_t
{
    Idle,
    Receiving,
    Failed,
    ReadyToBoot,
};

AsyncWebServer* gOtaServer = nullptr;
AsyncWebServerRequest* gOtaRequest = nullptr;
esp_ota_handle_t gOtaHandle = 0;
bool gOtaHandleValid = false;
const esp_partition_t* gpOtaPartition = nullptr;
size_t gOtaBytesReceived = 0;
OtaUploadStatus gOtaStatus = OtaUploadStatus::Idle;
esp_err_t gOtaError = ESP_OK;
String gOtaFailureReason;

void AbortOtaUpload()
{
    if (gOtaHandleValid)
    {
        esp_ota_abort(gOtaHandle);
        gOtaHandle = 0;
        gOtaHandleValid = false;
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
    return kOtaCustomJs;
}

void RegisterRoutes(AsyncWebServer* apServer)
{
    if (apServer == nullptr || apServer == gOtaServer)
    {
        return;
    }

    apServer->on("/update", HTTP_GET, [](AsyncWebServerRequest* apRequest)
    {
        apRequest->send(200, "text/html; charset=utf-8", kOtaPage);
    });

    apServer->on("/update", HTTP_POST,
        [](AsyncWebServerRequest* apRequest)
        {
            if (gOtaRequest != apRequest)
            {
                apRequest->send(400, "text/plain", "No firmware image was received or another upload is active.");
                return;
            }

            const OtaUploadStatus wStatus = gOtaStatus;
            const esp_err_t wError = gOtaError;
            const String wFailureReason = gOtaFailureReason;
            gOtaRequest = nullptr;
            gOtaStatus = OtaUploadStatus::Idle;
            gpOtaPartition = nullptr;
            gOtaBytesReceived = 0;
            gOtaFailureReason = "";

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
                if (gOtaRequest != nullptr && gOtaRequest != apRequest)
                {
                    return;
                }

                gOtaRequest = apRequest;
                gOtaStatus = OtaUploadStatus::Failed;
                gOtaError = ESP_FAIL;
                gOtaFailureReason = "Invalid firmware upload.";

                String wLowerFilename = arFilename;
                wLowerFilename.toLowerCase();
                if (!wLowerFilename.endsWith(".bin"))
                {
                    return;
                }

                gpOtaPartition = esp_ota_get_next_update_partition(nullptr);
                if (gpOtaPartition == nullptr)
                {
                    gOtaError = ESP_ERR_NOT_FOUND;
                    return;
                }

                gOtaError = esp_ota_begin(gpOtaPartition, OTA_SIZE_UNKNOWN, &gOtaHandle);
                if (gOtaError != ESP_OK)
                {
                    gOtaHandle = 0;
                    gOtaHandleValid = false;
                    return;
                }

                gOtaHandleValid = true;
                gOtaBytesReceived = 0;
                gOtaStatus = OtaUploadStatus::Receiving;
            }

            if (gOtaRequest != apRequest || gOtaStatus != OtaUploadStatus::Receiving)
            {
                return;
            }

            if (aLength > gpOtaPartition->size - gOtaBytesReceived)
            {
                AbortOtaUpload();
                gOtaStatus = OtaUploadStatus::Failed;
                gOtaError = ESP_ERR_INVALID_SIZE;
                return;
            }

            gOtaError = esp_ota_write(gOtaHandle, apData, aLength);
            if (gOtaError != ESP_OK)
            {
                AbortOtaUpload();
                gOtaStatus = OtaUploadStatus::Failed;
                return;
            }
            gOtaBytesReceived += aLength;

            if (aFinal)
            {
                gOtaError = esp_ota_end(gOtaHandle);
                gOtaHandle = 0;
                gOtaHandleValid = false;
                if (gOtaError != ESP_OK)
                {
                    gOtaFailureReason = "The uploaded file is not a valid ESP32 application image.";
                    gOtaStatus = OtaUploadStatus::Failed;
                    return;
                }

                ProductInfoNS::Record wNewRecord{};
                if (!ProductInfoNS::FindProductInfoInOtaImage(gpOtaPartition, gOtaBytesReceived, wNewRecord))
                {
                    gOtaError = ESP_ERR_INVALID_CRC;
                    gOtaFailureReason = "No valid embedded ProductInfo record was found in the firmware image.";
                    gOtaStatus = OtaUploadStatus::Failed;
                    return;
                }

                ProductInfoNS::tSoftwareVersion wCurrentVersion{};
                if (ProductInfoNS::IsInvalidSoftwareVersion(wNewRecord.softwareVersion))
                {
                    gOtaError = ESP_FAIL;
                    gOtaFailureReason = "Update refused: the selected firmware version is invalid (255.255.255).";
                    gOtaStatus = OtaUploadStatus::Failed;
                    return;
                }

                if (!ProductInfoNS::IsNewVersionAllowed(wNewRecord.softwareVersion, wCurrentVersion))
                {
                    char wMessage[128]{};
                    snprintf(wMessage, sizeof(wMessage),
                        "Update refused: firmware version %u.%u.%u is not newer than installed version %u.%u.%u.",
                        static_cast<unsigned>(wNewRecord.softwareVersion.mMajor),
                        static_cast<unsigned>(wNewRecord.softwareVersion.mMinor),
                        static_cast<unsigned>(wNewRecord.softwareVersion.mBuild),
                        static_cast<unsigned>(wCurrentVersion.mMajor),
                        static_cast<unsigned>(wCurrentVersion.mMinor),
                        static_cast<unsigned>(wCurrentVersion.mBuild));
                    gOtaFailureReason = wMessage;
                    gOtaError = ESP_FAIL;
                    gOtaStatus = OtaUploadStatus::Failed;
                    return;
                }

                gOtaError = esp_ota_set_boot_partition(gpOtaPartition);
                gOtaStatus = (gOtaError == ESP_OK)
                    ? OtaUploadStatus::ReadyToBoot
                    : OtaUploadStatus::Failed;
            }
        });

    gOtaServer = apServer;
}
}
