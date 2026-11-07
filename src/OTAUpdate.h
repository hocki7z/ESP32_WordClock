/*
 * OTAUpdate.h
 *
 *  Created on: 02.11.2026
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

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

    /* @brief Interface for OTA update status callbacks.
     *
     * @details
     * This interface defines a callback method that can be implemented
     * by classes interested in receiving notifications about OTA upload
     * status changes. Implementing classes should override the
     * OnOtaUploadStatusChanged method to handle status updates.
    */
    class OTAUpdateCallback
    {
    public:
        virtual void OnOtaUploadStatusChanged(OtaUploadStatus aStatus, esp_err_t aError, const String& arFailureReason) = 0;
    };

    const char* CustomJavaScript(void);

    void RegisterRoutes(AsyncWebServer* apServer, OTAUpdateCallback* apCallback = nullptr);
}
