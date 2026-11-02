#include "OTAUpdate.h"

#include <ESPAsyncWebServer.h>
#include <esp_ota_ops.h>
#include <esp_system.h>

#include <cstddef>
#include <cstdint>

namespace
{
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

        panel.append(picker, button, progress, status);
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
    return kOtaCustomJs;
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

                mOtaError = esp_ota_set_boot_partition(mpOtaPartition);
                mOtaStatus = (mOtaError == ESP_OK)
                    ? OtaUploadStatus::ReadyToBoot
                    : OtaUploadStatus::Failed;
            }
        });

    mpOtaServer = apServer;
}
}
