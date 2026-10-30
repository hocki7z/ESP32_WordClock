/*
 * WebSite.cpp
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#include <ESPUI.h>
#include <esp_ota_ops.h>
#include <esp_system.h>

#include "Logger.h"
#include "Configuration.h"
#include "DateTime.h"
#include "BitMatrix.h"
#include "DisplayLayout.h"
#include "Settings.hpp"

#include "WebSite.h"


/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)

/* CSS styles for the web UI controls */
#define CSS_STYLE_NONE                  ""
#define CSS_STYLE_GROUP                 "background-color: unset; width: 100%; text-align: center;"
#define CSS_STYLE_SEPARATOR_GAP         "background-color: unset; width: 100%; height: 4px; min-height: 4px; box-sizing: border-box; line-height: 0; padding: 0; margin: 0;"
#define CSS_STYLE_SEPARATOR_LINE        "background-color: unset; width: 100%; height: 4px; min-height: 4px; box-sizing: border-box; padding: 0; margin: 0; border-top: 1px solid #808080;"
#define CSS_STYLE_LABEL_DEFAULT         "background-color: unset; text-align: left; padding: 0; margin-bottom: 4px; top: 0; vertical-align: bottom;"
#define CSS_STYLE_LABEL_H2              CSS_STYLE_LABEL_DEFAULT "width: 100%;"
#define CSS_STYLE_LABEL                 CSS_STYLE_LABEL_DEFAULT "font-weight: normal;  width: 100%;"
#define CSS_STYLE_LABEL_LIGHTER         CSS_STYLE_LABEL_DEFAULT "font-weight: 300; font-size: 0.9em; width: 100%;"
#define CSS_STYLE_LABEL_LIGHTER_HALF    CSS_STYLE_LABEL_DEFAULT "font-weight: 300; font-size: 0.9em; width: 50%;"
#define CSS_STYLE_INPUT_DEFAULT         "color: black; height: 26px; margin-left: 8px; box-sizing: border-box;"
#define CSS_STYLE_INPUT                 CSS_STYLE_INPUT_DEFAULT "width: calc(100% - 8px);"
#define CSS_STYLE_INPUT_HALF_WIDTH      CSS_STYLE_INPUT_DEFAULT "width: calc(50% - 8px);" // display: inline-block;
#define CSS_STYLE_SLIDER                "margin-left: 8px; width: calc(100% - 53px); box-sizing: border-box;"
#define CSS_STYLE_PREVIEW               "background-color: #000000; padding: 0; width: 100%; text-align: center;"
#define CSS_STYLE_SWITCH                "margin-bottom: 10px; align-items: center; text-align: right; vertical-align: middle;" // float: right; margin: 0 8px 10px 0;
#define CSS_STYLE_SWITCH_LABEL          "background-color: unset; font-weight: normal; width: calc(100% - 68px); text-align: left; padding: 0; vertical-align: middle;"
#define CSS_STYLE_BUTTON                "width: 100%" //; font-size:1.6rem;min-height:4rem;padding:1rem;margin:3px;border:2px solid #404040;border-radius:5px;"

/* Delay for web UI updates in milliseconds */
#define WEB_UI_UPDATE_DELAY             100     // It seems that a small delay is enough to avoid issues with the web UI updates, especially when updating control states

/* Default control colors */
#define TAB_CONTROL_COLOR               Control::Color::Wetasphalt
#define GROUP_CONTROL_COLOR             Control::Color::Wetasphalt

namespace
{
constexpr char kOtaPage[] = R"html(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Wordclock firmware update</title><style>
body{font:16px system-ui,sans-serif;max-width:640px;margin:3rem auto;padding:0 1rem;background:#111827;color:#f3f4f6}
main{background:#1f2937;padding:2rem;border-radius:16px}h1{margin-top:0}input,button{font:inherit;margin:.5rem 0;padding:.7rem;width:100%;box-sizing:border-box}
button{background:#2563eb;color:white;border:0;border-radius:8px;cursor:pointer}button:disabled{opacity:.5}
progress{width:100%;height:1.2rem}#status{min-height:1.5rem;white-space:pre-wrap}
</style></head><body><main><h1>Firmware update</h1>
<p>Select the <strong>firmware_productinfo_ota.bin</strong> file produced by the PlatformIO build.</p>
<form id="form"><label>Firmware image (.bin)<input id="firmware" type="file" accept=".bin,application/octet-stream" required></label>
<button id="submit" type="submit">Upload and install</button></form>
<progress id="progress" value="0" max="100" hidden></progress><p id="status" role="status"></p>
<script>
const form=document.getElementById('form'),button=document.getElementById('submit'),progress=document.getElementById('progress'),status=document.getElementById('status');
form.addEventListener('submit',event=>{event.preventDefault();const file=document.getElementById('firmware').files[0];
if(!file||!file.name.toLowerCase().endsWith('.bin')){status.textContent='Select a .bin application image.';return;}
button.disabled=true;progress.hidden=false;progress.value=0;status.textContent='Uploading…';const data=new FormData();data.append('firmware',file,file.name);
const xhr=new XMLHttpRequest();xhr.open('POST','/update');xhr.upload.onprogress=e=>{if(e.lengthComputable){progress.value=Math.round(e.loaded*100/e.total);status.textContent='Uploading '+progress.value+'%';}};
xhr.onload=()=>{status.textContent=xhr.responseText||('Update failed ('+xhr.status+').');if(xhr.status!==200)button.disabled=false;};xhr.onerror=()=>{status.textContent='Connection lost. The device may be restarting; wait and reconnect.';button.disabled=false;};xhr.send(data);});
</script></main></body></html>)html";

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
        description.textContent = 'Select firmware_productinfo_ota.bin from your PlatformIO build output.';
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
            progress.hidden = false;
            progress.value = 0;
            status.textContent = 'Uploading…';

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

void RegisterOtaRoutes()
{
    AsyncWebServer* const wpServer = ESPUI.WebServer();
    if (wpServer == nullptr || wpServer == gOtaServer)
    {
        return;
    }

    wpServer->on("/update", HTTP_GET, [](AsyncWebServerRequest* apRequest)
    {
        apRequest->send(200, "text/html; charset=utf-8", kOtaPage);
    });

    wpServer->on("/update", HTTP_POST,
        [](AsyncWebServerRequest* apRequest)
        {
            if (gOtaRequest != apRequest)
            {
                apRequest->send(400, "text/plain", "No firmware image was received or another upload is active.");
                return;
            }

            const OtaUploadStatus wStatus = gOtaStatus;
            const esp_err_t wError = gOtaError;
            gOtaRequest = nullptr;
            gOtaStatus = OtaUploadStatus::Idle;
            gpOtaPartition = nullptr;
            gOtaBytesReceived = 0;

            if (wStatus == OtaUploadStatus::ReadyToBoot)
            {
                apRequest->send(200, "text/plain", "Firmware accepted. The device is restarting now.");
                xTaskCreate(RestartAfterOta, "ota_restart", 2048, nullptr, 1, nullptr);
                return;
            }

            apRequest->send(400, "text/plain",
                String("Firmware update failed (ESP error ") + String(static_cast<int>(wError)) + ").");
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
                    gOtaStatus = OtaUploadStatus::Failed;
                    return;
                }

                gOtaError = esp_ota_set_boot_partition(gpOtaPartition);
                gOtaStatus = (gOtaError == ESP_OK)
                    ? OtaUploadStatus::ReadyToBoot
                    : OtaUploadStatus::Failed;
            }
        });

    gOtaServer = wpServer;
}
}

/**
 * Initialize the private static pointer
 */
WebSite* WebSite::mpWebSiteInstance = nullptr;


/**
 * @brief Constructor
 */
WebSite::WebSite(char const* apName, ApplicationNS::tTaskPriority aPriority, const uint32_t aStackSize)
    : ApplicationNS::Task(apName, aPriority, aStackSize)
{
    /* Set "this" static pointer */
    mpWebSiteInstance = this;
}

/**
 * @brief Destructor
 */
WebSite::~WebSite()
{
    /* Delete the control event queue and notification */
    delete mpControlEventQueue;
    delete mpControlEventNotification;

    mpControlEventQueue = nullptr;
    mpControlEventNotification = nullptr;

    /* Clear "this" static pointer */
    mpWebSiteInstance = nullptr;
}

void WebSite::Init(ApplicationNS::tTaskObjects* apTaskObjects)
{
    /* Initialize base class */
    ApplicationNS::Task::Init(apTaskObjects);

    /* Create a queue for ESPUI control events */
    mpControlEventQueue = new ControlEventQueue();
    if (mpControlEventQueue == nullptr)
    {
        LOG(LOG_ERROR, "WebSite::Init() Failed to create control event queue");
    }

    /* Create a task notification for control events */
    mpControlEventNotification = new TaskNotification(getTaskHandle(), mTaskNotificationControlEvent);
    if (mpControlEventNotification == nullptr)
    {
        LOG(LOG_ERROR, "WebSite::Init() Failed to create control event notification");
    }

    /*
     * Initialize WEB UI but do not start it yet
     */

    /* ESPUI log mode - turn on/off verbose debugging */
    ESPUI.setVerbosity((LOG_LEVEL == LOG_VERBOSE) ? Verbosity::Verbose : Verbosity::Quiet);

	/* Make sliders continually report their position as they are being dragged. */
	ESPUI.sliderContinuous = true;

    /* Add the tabs for the web UI */
    mWebUIControlID.mOverViewTab   = ESPUI.addControl(Control::Type::Tab, "overview",    "Overview",    TAB_CONTROL_COLOR);
    mWebUIControlID.mLedDisplayTab = ESPUI.addControl(Control::Type::Tab, "led-display", "LED Display", TAB_CONTROL_COLOR);
    mWebUIControlID.mSettingsTab   = ESPUI.addControl(Control::Type::Tab, "settings",    "Settings",    TAB_CONTROL_COLOR);


    /**
     * Tab Overview / Group Preview
     */
    mWebUIControlID.mPreviewGroup = AddGroupHelper("Preview", mWebUIControlID.mOverViewTab, GROUP_CONTROL_COLOR);

    /* Live preview of the word-clock LED matrix */
    mWebUIControlID.mDisplayPreview = AddLabelControl("", mWebUIControlID.mPreviewGroup, CSS_STYLE_PREVIEW,
        BuildDisplayPreviewSvg()); // "Here will be a live preview of the word-clock LED matrix. Please wait until the first update is received from the device


    /**
     * Tab LED Display / Group Display
     */
    mWebUIControlID.mLedDisplayDisplayGroup = AddGroupHelper("Display", mWebUIControlID.mLedDisplayTab, GROUP_CONTROL_COLOR);

    /* Clock mode */
    AddLabelControl("", mWebUIControlID.mLedDisplayDisplayGroup, CSS_STYLE_LABEL, "Clock mode");
    mWebUIControlID.mDisplayClockMode = AddSelectControl("", mWebUIControlID.mLedDisplayDisplayGroup, CSS_STYLE_INPUT,
            mcClockModeItems, WORDCLOCK_MODE_NUMBER,
            ConfigNS::mKeyDisplayClockMode, mcDefaultDisplayClockMode);

    /* Switcher for 'IT IS' words */
    AddLabelControl("", mWebUIControlID.mLedDisplayDisplayGroup, CSS_STYLE_SWITCH_LABEL, "Show 'IT IS'");
    mWebUIControlID.mDisplayClockItIs = AddSwitcherControl("", mWebUIControlID.mLedDisplayDisplayGroup, CSS_STYLE_SWITCH,
            ConfigNS::mKeyDisplayClockItIs, ConfigNS::mDefaultDisplayClockItIs);

    /* Switch for single minutes */
    AddLabelControl("", mWebUIControlID.mLedDisplayDisplayGroup, CSS_STYLE_SWITCH_LABEL, "Show single minutes");
    mWebUIControlID.mDisplayClockSingleMinutes = AddSwitcherControl("", mWebUIControlID.mLedDisplayDisplayGroup, CSS_STYLE_SWITCH,
            ConfigNS::mKeyDisplayClockSingleMins, ConfigNS::mDefaultDisplayClockSingleMins);


    /**
     * Tab LED Display / Group Color
     */
    mWebUIControlID.mLedDisplayColorGroup = AddGroupHelper("Colors", mWebUIControlID.mLedDisplayTab, GROUP_CONTROL_COLOR);

    /* Day time and background colors */
    AddLabelControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_LABEL, "Day");
    AddLabelControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_LABEL_LIGHTER_HALF, "time");
    AddLabelControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_LABEL_LIGHTER_HALF, "background");

    mWebUIControlID.mDisplayColorTime = AddColorControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_INPUT_HALF_WIDTH,
        ConfigNS::mKeyDisplayColorTime, ConfigNS::mDefaultDisplayColorTime);
    mWebUIControlID.mDisplayColorBkgd = AddColorControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_INPUT_HALF_WIDTH,
        ConfigNS::mKeyDisplayColorBkgd, ConfigNS::mDefaultDisplayColorBkgd);

    /* Separator */
    AddSeparator(mWebUIControlID.mLedDisplayColorGroup);

    /* Night time and background colors */
    AddLabelControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_LABEL, "Night");
    AddLabelControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_LABEL_LIGHTER_HALF, "time");
    AddLabelControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_LABEL_LIGHTER_HALF, "background");

    mWebUIControlID.mDisplayColorTimeNight = AddColorControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_INPUT_HALF_WIDTH,
        ConfigNS::mKeyDisplayColorTimeNight, ConfigNS::mDefaultDisplayColorTimeNight);
    mWebUIControlID.mDisplayColorBkgdNight = AddColorControl("", mWebUIControlID.mLedDisplayColorGroup, CSS_STYLE_INPUT_HALF_WIDTH,
        ConfigNS::mKeyDisplayColorBkgdNight, ConfigNS::mDefaultDisplayColorBkgdNight);


    /**
     * Tab LED Display / Group Brightness
     */
    mWebUIControlID.mLedDisplayBrightnessGroup = AddGroupHelper("Brightness", mWebUIControlID.mLedDisplayTab, GROUP_CONTROL_COLOR);

    /* Switcher for day/night mode activation */
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_SWITCH_LABEL, "Use day/night mode");
    mWebUIControlID.mDisplayUseDayNightMode = AddSwitcherControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_SWITCH,
            ConfigNS::mKeyDisplayUseDayNightMode, ConfigNS::mDefaultDisplayUseDayNightMode);

    /* Time input for day mode start/end */
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL, "Day mode");
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL_LIGHTER_HALF, "start time");
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL_LIGHTER_HALF, "end time");
    mWebUIControlID.mDisplayDayModeStartTime = AddTimeControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_INPUT_HALF_WIDTH,
            ConfigNS::mKeyDisplayDayModeStartTime, ConfigNS::mDefaultDisplayDayModeStartTime);
    mWebUIControlID.mDisplayDayModeEndTime = AddTimeControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_INPUT_HALF_WIDTH,
            ConfigNS::mKeyDisplayDayModeEndTime, ConfigNS::mDefaultDisplayDayModeEndTime);

    AddSeparator(mWebUIControlID.mLedDisplayBrightnessGroup, true); // Draw a line separator

    /* Sliders for day time/background brightness */
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL, "Day");
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL_LIGHTER, "time");
    mWebUIControlID.mDisplayBrightnessTime = AddPercentageSliderControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_SLIDER,
        ConfigNS::mKeyDisplayBrightnessTime, ConfigNS::mDefaultDisplayBrightnessTime);
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL_LIGHTER, "background");
    mWebUIControlID.mDisplayBrightnessBkgd = AddPercentageSliderControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_SLIDER,
            ConfigNS::mKeyDisplayBrightnessBkgd, ConfigNS::mDefaultDisplayBrightnessBkgd);

    AddSeparator(mWebUIControlID.mLedDisplayBrightnessGroup);

    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL, "Night");
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL_LIGHTER, "time");
    mWebUIControlID.mDisplayBrightnessTimeNight = AddPercentageSliderControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_SLIDER,
        ConfigNS::mKeyDisplayBrightnessTimeNight, ConfigNS::mDefaultDisplayBrightnessTimeNight);
    AddLabelControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_LABEL_LIGHTER, "background");
    mWebUIControlID.mDisplayBrightnessBkgdNight = AddPercentageSliderControl("", mWebUIControlID.mLedDisplayBrightnessGroup, CSS_STYLE_SLIDER,
        ConfigNS::mKeyDisplayBrightnessBkgdNight, ConfigNS::mDefaultDisplayBrightnessBkgdNight);


    /**
     * Tab Settings / Group Time
     */
    mWebUIControlID.mSettingsTimeGroup = AddGroupHelper("Time", mWebUIControlID.mSettingsTab, GROUP_CONTROL_COLOR);

    /* NTP server selection */
    AddLabelControl("", mWebUIControlID.mSettingsTimeGroup, CSS_STYLE_LABEL, "NTP server");
    mWebUIControlID.mDatetimeNtpServer = AddSelectControl("", mWebUIControlID.mSettingsTimeGroup, CSS_STYLE_INPUT,
            ConfigNS::mcNtpServerItems, ConfigNS::mcNtpServerItemsCount,
            ConfigNS::mKeyNtpServer, ConfigNS::mDefaultNtpServer);
    /* Timezone selection */
    AddLabelControl("", mWebUIControlID.mSettingsTimeGroup, CSS_STYLE_LABEL, "Time zone");
    mWebUIControlID.mDatetimeTimeZone = AddSelectControl("", mWebUIControlID.mSettingsTimeGroup, CSS_STYLE_INPUT,
            ConfigNS::mcTimezoneNames, ConfigNS::mcTimezoneItemsCount,
            ConfigNS::mKeyTimeZone, ConfigNS::mDefaultTimeZone);


    /**
     * Tab Settings / Group WiFi
     */
    mWebUIControlID.mSettingsWiFiGroup = AddGroupHelper("WiFi Access Point Credentials", mWebUIControlID.mSettingsTab, GROUP_CONTROL_COLOR);

    /* WiFi settings controls here (e.g., SSID, password, etc.) */
    AddLabelControl("", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_LABEL, "SSID");
    mWebUIControlID.mWifiSSIDs = AddSelectControl("", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_INPUT,
        nullptr, 0, ConfigNS::mInvalidKey, 0, ControlHandler::WifiSsidSelect);

    AddLabelControl("", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_LABEL, "Password");
    mWebUIControlID.mWifiPassword = AddPasswordControl("", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_INPUT,
        ConfigNS::mInvalidKey, ControlHandler::WifiPassword);

    AddLabelControl("", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_SWITCH_LABEL, "Show/Hide Password");
    mWebUIControlID.mWifiPasswordShowHide = AddSwitcherControl("", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_SWITCH,
        ConfigNS::mInvalidKey, false, ControlHandler::WifiPasswordShowHide);

    /* Add buttons for scanning WiFi networks and connecting to the selected network */
    mWebUIControlID.mWifiConnectButton = AddButtonControl("", "Save & Connect", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_BUTTON,
        ControlHandler::WifiConnectButton);

    mWebUIControlID.mWifiScanButton = AddButtonControl("", "Search for WiFi", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_BUTTON,
        ControlHandler::WifiScanButton);
    mWebUIControlID.mWifiForgetButton = AddButtonControl("", "Forget network", mWebUIControlID.mSettingsWiFiGroup, CSS_STYLE_BUTTON,
        ControlHandler::WifiForgetButton);


    /**
     * Tab Settings / Group Product Information
     */
    const Control::ControlId_t wProductInfoGroup =
        AddGroupHelper("Product Information", mWebUIControlID.mSettingsTab, GROUP_CONTROL_COLOR);

    ProductInfoNS::Record wProductInfo{};
    String wSoftwareVersion = "Unavailable";
    String wBuildDateTime = "Unavailable";
    String wCommitHash = "Unavailable";
    String wBranch = "Unavailable";

    if (ConfigNS::GetProductInfo(wProductInfo))
    {
        if ((wProductInfo.softwareVersion.mMajor == 0xFF) &&
            (wProductInfo.softwareVersion.mMinor == 0xFF) &&
            (wProductInfo.softwareVersion.mBuild == 0xFF))
        {
            wSoftwareVersion = "Unknown";
        }
        else
        {
            char wVersionText[12] = {0};
            snprintf(wVersionText, sizeof(wVersionText), "%u.%u.%u",
                static_cast<unsigned>(wProductInfo.softwareVersion.mMajor),
                static_cast<unsigned>(wProductInfo.softwareVersion.mMinor),
                static_cast<unsigned>(wProductInfo.softwareVersion.mBuild));
            wSoftwareVersion = wVersionText;
        }

        wBuildDateTime = wProductInfo.buildDateTimeUtc;
        if (wProductInfo.commitHashShort[0] != '\0')
        {
            wCommitHash = wProductInfo.commitHashShort;
        }
        if (wProductInfo.branchShort[0] != '\0')
        {
            wBranch = wProductInfo.branchShort;
        }
    }

    AddLabelControl("Software version", wProductInfoGroup, CSS_STYLE_LABEL, wSoftwareVersion);
    AddLabelControl("Build date/time (UTC)", wProductInfoGroup, CSS_STYLE_LABEL, wBuildDateTime);
    AddLabelControl("Commit hash", wProductInfoGroup, CSS_STYLE_LABEL, wCommitHash);
    AddLabelControl("Branch", wProductInfoGroup, CSS_STYLE_LABEL, wBranch);

    const Control::ControlId_t wFirmwareUpdateGroup =
        AddGroupHelper("Firmware Update", mWebUIControlID.mSettingsTab, GROUP_CONTROL_COLOR);
}

void WebSite::ProcessIncomingMessage(const MessageNS::Message &arMessage)
{
    LOG(LOG_VERBOSE, "WebSite::ProcessIncomingMessage()");

    switch (arMessage.mId)
    {
        case MessageNS::tMessageId::MSG_EVENT_WIFI_STA_CONNECTED:
            /* WiFi connected, start WEB server */
            LOG(LOG_DEBUG, "WebSite::ProcessIncomingMessage() Start web server");

            /* Normal Mode - no captive portal */
            ESPUI.captivePortal = false;

            /* Update WiFi settings controls */
            UpdateWiFiSettingsControls();

            /* Start WEB UI */
            ESPUI.setCustomJS(kOtaCustomJs);
            ESPUI.begin("Wordclock");
            RegisterOtaRoutes();
            break;

        case MessageNS::tMessageId::MSG_EVENT_WIFI_AP_STARTED:
            /* Access point started, start WEB server */
            LOG(LOG_DEBUG, "WebSite::ProcessIncomingMessage() Start web server in AP mode");

            /* Offline Mode - enable captive portal */
            ESPUI.captivePortal = true;

            /* Update WiFi settings controls */
            UpdateWiFiSettingsControls();

            /* Start WEB UI */
            ESPUI.setCustomJS(kOtaCustomJs);
            ESPUI.begin("Wordclock");
            RegisterOtaRoutes();
            break;

        case MessageNS::tMessageId::MSG_EVENT_WIFI_SCAN_DONE:
            /* WiFi scan finished, update WiFi settings */
            UpdateWiFiSettingsControls();
            break;

        case MessageNS::tMessageId::MSG_EVENT_DISPLAY_FRAME_READY:
            UpdateDisplayPreview(arMessage);
            break;

        default:
            // do nothing
            break;
    }
}

void WebSite::ProcessUnknownNotification(uint32_t aNotificationValue)
{
    /* Process control event notification */
    if ((aNotificationValue & mTaskNotificationControlEvent) != 0)
    {
        ControlEvent wEvent{};

        /* Process all pending control events */
        while (mpControlEventQueue->pop(wEvent, 0))
        {
            switch (wEvent.mControlParam.mHandler)
            {
                /* Handle WiFi controls */
                case ControlHandler::WifiSsidSelect:
                case ControlHandler::WifiPassword:
                case ControlHandler::WifiPasswordShowHide:
                case ControlHandler::WifiConnectButton:
                case ControlHandler::WifiScanButton:
                case ControlHandler::WifiForgetButton:
                    HandleWiFiControlEvent(wEvent);
                    break;

                /* Handle other controls */
                default:
                    HandleControlEvent(wEvent);
                    break;
            }

            /* Allow other tasks to run */
            yield();
        }

        /* Clear the control event notification bit */
        aNotificationValue &= ~mTaskNotificationControlEvent;
    }
}

/** @brief Handle a control event
 *
 * @param arEvent The control event to handle
 */
void WebSite::HandleControlEvent(const ControlEvent& arEvent)
{
    /* Flag to indicate whether to send a message or not */
    bool wSendMessage = false;  // default - false

    SettingsNS::tKey wSettingsKey = arEvent.mControlParam.mSettingsKey;

    if (wSettingsKey != ConfigNS::mInvalidKey)
    {
        /* Handle control based on its handler tag */
        switch (arEvent.mControlParam.mHandler)
        {
            case ControlHandler::Text:
            {
                /* LOG */
                LOG(LOG_DEBUG, "WebSite::HandleControlEvent() Text control %04X, new value %s",
                        arEvent.mControlId, arEvent.mPayload.mParameter.mValueText);

                /* Store new text value in settings */
                wSendMessage = Settings.SetValue<String>(wSettingsKey, arEvent.mPayload.mParameter.mValueText);
            }
                break;

            case ControlHandler::Color:
            {
                /* Retrieve the color value */
                std::string wColorStr = arEvent.mPayload.mParameter.mValueText;
                uint32_t  wColorValue = std::stoi(wColorStr.substr(1), nullptr, 16); // skip '#'

                /* Update displayed value */
                ESPUI.updateText(arEvent.mControlId, arEvent.mPayload.mParameter.mValueText);

                /* LOG */
                LOG(LOG_DEBUG, "WebSite::HandleControlEvent() Color control %04X, new color %s (0x%08X)",
                        arEvent.mControlId, wColorStr.c_str(), wColorValue);

                /* Store new color value as uint32_t in settings */
                wSendMessage = Settings.SetValue<uint32_t>(wSettingsKey, wColorValue);
            }
                break;

            case ControlHandler::Timer:
            {
                /* Retreive new time value in format HH:MM */
                std::string wTimeStr = arEvent.mPayload.mParameter.mValueText;

                uint8_t wHour   = static_cast<uint8_t>(std::stoi(wTimeStr.substr(0, 2)));
                uint8_t wMinute = static_cast<uint8_t>(std::stoi(wTimeStr.substr(3, 2)));

                /* LOG */
                LOG(LOG_DEBUG, "WebSite::HandleControlEvent() Timer control %04X, new time %02u:%02u",
                        arEvent.mControlId, wHour, wMinute);

                /* Convert to uint32_t in format HHMM */
                DateTimeNS::tDateTime wDateTime;
                wDateTime.mTime.mHour   = wHour;
                wDateTime.mTime.mMinute = wMinute;
                wDateTime.mTime.mSecond = 0;
                wDateTime.mDate.mDay    = 1;
                wDateTime.mDate.mMonth  = 1;
                wDateTime.mDate.mYear   = DateTimeNS::mYearRangeStart;

                uint32_t wTimeDword = DateTimeNS::DateTimeToDword(wDateTime);

                /* Store new time value as uint32_t in settings */
                wSendMessage = Settings.SetValue<uint32_t>(wSettingsKey, wTimeDword);
            }
                break;

            case ControlHandler::Switcher:
            {
                /* Retrieve new switcher state */
                bool wState = (arEvent.mPayload.mParameter.mValue != 0);

                LOG(LOG_DEBUG, "WebSite::HandleControlEvent() Switcher control %04X, new state %d",
                        arEvent.mControlId, wState);

                /* Store new text value as bool in settings*/
                wSendMessage = Settings.SetValue<bool>(wSettingsKey, wState);
            }
                break;

            case ControlHandler::Select:
            {
                /* Retrieve new selected option */
                uint8_t wSelectedOption = static_cast<uint8_t>(arEvent.mPayload.mParameter.mValue);

                /* LOG */
                LOG(LOG_DEBUG, "WebSite::HandleControlEvent() Select control %04X, type %d, selection %d",
                        arEvent.mControlId, arEvent.mEventType, wSelectedOption);

                /* Store new selected option as uint8_t in settings */
                wSendMessage = Settings.SetValue<uint8_t>(wSettingsKey, wSelectedOption);
            }
                break;

            case ControlHandler::Slider:
            {
                /* Retrieve new slider value */
                uint8_t wValue = static_cast<uint8_t>(arEvent.mPayload.mParameter.mValue);

                /* LOG */
                LOG(LOG_DEBUG, "WebSite::HandleControlEvent() Slider control %04X, new value %d",
                        arEvent.mControlId, wValue);

                /* Store new slider value as uint8_t in settings */
                wSendMessage = Settings.SetValue<uint8_t>(wSettingsKey, wValue);
            }
                break;

            default:
                /* LOG */
                LOG(LOG_ERROR, "WebSite::HandleControl() Unknown control handler for control ID %04X", arEvent.mControlId);
                break;
        }
    }

    /* Send message if needed */
    if (wSendMessage)
    {
        /* Prepare message */
        MessageNS::Message wMessage;

        wMessage.mSource = MessageNS::tAddress::WEB_MANAGER;
        wMessage.mId = MessageNS::tMessageId::MSG_EVENT_SETTINGS_CHANGED;

        /* Send message to display */
        wMessage.mDestination = MessageNS::tAddress::DISPLAY_MANAGER;
        SendMessage(wMessage);

        /* Send message to time manager */
        wMessage.mDestination = MessageNS::tAddress::TIME_MANAGER;
        SendMessage(wMessage);
    }
}

/** @brief Handle a WiFi control event
 *
 * @param arEvent The WiFi control event to handle
 */
void WebSite::HandleWiFiControlEvent(const ControlEvent& arEvent)
{
    /* Message to be sent if control changes */
    MessageNS::Message wMessage;
    /* Flag to indicate whether to send a message or not */
    bool wSendMessage = false;  // default - false

    /* Prepare message */
    wMessage.mSource = MessageNS::tAddress::WEB_MANAGER;
    wMessage.mDestination = MessageNS::tAddress::WIFI_MANAGER;

    /* Handle control based on its handler tag */
    switch (arEvent.mControlParam.mHandler)
    {
        case ControlHandler::WifiSsidSelect:
        case ControlHandler::WifiPassword:
        {
            bool wIsSsidValid     = false;
            bool wIsPasswordValid = false;
            tSSIDEntry wSelectedSsid{};

            /* Get current WiFi SSID selection */
            const long wSsidIndex = arEvent.mPayload.mWifi.mSsidIndex;
            /* Use the password snapshot captured with this event. */
            String wPassw = arEvent.mPayload.mWifi.mPassword;

            wIsSsidValid = (wSsidIndex >= 0) &&
                (static_cast<size_t>(wSsidIndex) < mcMaxSsidCount) &&
                mSSIDList.Get(static_cast<size_t>(wSsidIndex), wSelectedSsid) &&    // Copy the selected entry
                wSelectedSsid.IsValid();

            if (wIsSsidValid)
            {
                /* Allows any valid SSID in captive-portal mode;
                 * otherwise, excludes index 0 (the current network) */
                wIsSsidValid = ESPUI.captivePortal || (wSsidIndex > 0);

                /* Check password validity: it is non-empty and the selected network is encrypted */
                wIsPasswordValid = ((wPassw.length() > 7) && wSelectedSsid.mEncrypted);    // Technical minimum length: 8 characters
            }

            ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton, (wIsSsidValid && wIsPasswordValid));

        }
            break;

        case ControlHandler::WifiPasswordShowHide:
        {
            /* WiFi password show/hide switcher changed */
            bool wState = (arEvent.mEventType == S_ACTIVE);

            /* Set the input type of the password field based on the switcher state */
            ESPUI.setInputType(mWebUIControlID.mWifiPassword, wState ? "text" : "password");

            /* Wait a bit to avoid issues with the UI update */
            delay(WEB_UI_UPDATE_DELAY);
        }
            break;

        case ControlHandler::WifiConnectButton:
        {
            /* Connect to selected network button pressed */
            if (arEvent.mEventType == B_UP)
            {
                LOG(LOG_DEBUG, "WebSite::HandleControl() WiFi connect button pressed");

                /* Get selected SSID and password from the controls */
                const long wSsidIndex = arEvent.mPayload.mWifi.mSsidIndex;
                tSSIDEntry wSelectedSsid{};
                const bool wIsSsidValid = (wSsidIndex >= 0) &&
                    (static_cast<size_t>(wSsidIndex) < mcMaxSsidCount) &&
                    mSSIDList.Get(static_cast<size_t>(wSsidIndex), wSelectedSsid) &&
                    wSelectedSsid.IsValid();
                if (!wIsSsidValid)
                {
                    LOG(LOG_ERROR, "WebSite::HandleWiFiControls() Ignoring connect request with invalid SSID index %ld", wSsidIndex);
                    break;
                }

                String wSsid = wSelectedSsid.mSsid;
                String wPassw = arEvent.mPayload.mWifi.mPassword;

                LOG(LOG_DEBUG, "WebSite::HandleWiFiControls() Connecting to SSID: %s", wSsid.c_str());

                /* Store SSID and password in settings */
                Settings.SetValue<String>(ConfigNS::mKeyWifiSSID, wSsid.c_str());
                Settings.SetValue<String>(ConfigNS::mKeyWifiPassword, wPassw.c_str());

                /* Message to WiFi manager with connect command */
                wMessage.mId = MessageNS::tMessageId::CMD_WIFI_CONNECT;
                wSendMessage = true;
            }
        }
            break;

        case ControlHandler::WifiScanButton:
        {
            /* Scan WiFi networks button pressed */
            if (arEvent.mEventType == B_UP)
            {
                LOG(LOG_DEBUG, "WebSite::HandleControl() WiFi scan button pressed");

                /* Update scan button label with ongoing status */
                ESPUI.updateLabel(mWebUIControlID.mWifiScanButton, "WiFi scan ongoing, please wait...");

                /* Disable WiFi settings controls during scanning */
                ESPUI.setEnabled(mWebUIControlID.mWifiSSIDs,            false);
                ESPUI.setEnabled(mWebUIControlID.mWifiPassword,         false);
                ESPUI.setEnabled(mWebUIControlID.mWifiPasswordShowHide, false);
                ESPUI.setEnabled(mWebUIControlID.mWifiForgetButton,     false);
                ESPUI.setEnabled(mWebUIControlID.mWifiScanButton,       false);
                ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton,    false);

                /* Wait a bit to avoid issues with the UI update */
                delay(WEB_UI_UPDATE_DELAY);

                /* Message to WiFi manager start scan command */
                wMessage.mId = MessageNS::tMessageId::CMD_WIFI_START_SCAN;
                wSendMessage = true;
            }
        }
            break;

        case ControlHandler::WifiForgetButton:
        {
            /* Forget WiFi network button pressed */
            if (arEvent.mEventType == B_UP)
            {
                LOG(LOG_DEBUG, "WebSite::HandleControl() WiFi forget button pressed");

                /* Remove SSID and password from settings */
                Settings.RemoveKey(ConfigNS::mKeyWifiSSID);
                Settings.RemoveKey(ConfigNS::mKeyWifiPassword);

                /* Message to WiFi manager with connect command */
                wMessage.mId = MessageNS::tMessageId::CMD_WIFI_CONNECT;
                wSendMessage = true;
            }
        }
            break;

        default:
            /* LOG */
            LOG(LOG_ERROR, "WebSite::HandleWiFiControls() Unknown control handler %d",
                static_cast<int>(arEvent.mControlParam.mHandler));
            break;
    }

    /* Send message if needed */
    if (wSendMessage)
    {
        SendMessage(wMessage);
    }
}

/**
 * @brief Helper function to add a group control to the web UI
 * @param apLabel The label for the group control
 * @param aParent The parent control ID (default: no parent)
 * @param aColor The color of the group control (default: None)
 * @param aExpandable Whether the group control is expandable (default: false)
 * @param aExpanded Whether an expandable group starts open (default: false)
 *
 * @return The control ID of the newly added group control
 */
Control::ControlId_t WebSite::AddGroupHelper(const char * apLabel, Control::ControlId_t aParent, Control::Color aColor,
    const bool aExpandable, const bool aExpanded)
{
	Control::ControlId_t wControlId = Control::noParent;

    if (aExpandable)
    {
        wControlId = ESPUI.addControl(Control::Type::Details, apLabel, "", aColor, aParent);

        ESPUI.updateDetails(wControlId, aExpanded);
    }
    else
    {
        wControlId = ESPUI.addControl(Control::Type::Label, apLabel, "", aColor, aParent);
    }

	ESPUI.setElementStyle(wControlId, CSS_STYLE_GROUP);

    return wControlId;
}

/**
 * @brief Helper function to add a label control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param arValue The initial value of the label (default: empty string)
 *
 * @return The control ID of the newly added label control
 */
Control::ControlId_t WebSite::AddLabelControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle, const String& arValue)
{
    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Label, apLabel, arValue, Control::Color::None, aParent);
    ESPUI.setElementStyle(wControlId, aElementStyle);

    return wControlId;
}

/**
 * @brief Add a visual separator or a small gap to the web UI
 * @param aParent The parent control ID
 * @param aDrawLine True to draw a horizontal line; false to add a small gap
 * @return The control ID of the separator label
 */
Control::ControlId_t WebSite::AddSeparator(Control::ControlId_t aParent, bool aDrawLine)
{
    const char* wpStyle = aDrawLine ? CSS_STYLE_SEPARATOR_LINE : CSS_STYLE_SEPARATOR_GAP;
    return AddLabelControl("", aParent, wpStyle, "");
}

/**
 * @brief Helper function to add a text control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param arValue The initial value of the text control (default: empty string)
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aHandler The control handler for the control (default: None)
 *
 * @return The control ID of the newly added text control
 */
Control::ControlId_t WebSite::AddTextControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle, const String& arValue,
        SettingsNS::tKey aSettingsKey, ControlHandler aHandler)
{
    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Text, apLabel, arValue, Control::Color::Dark, aParent,
            WebSite::ControlCallback, new ControlParam{aHandler, aSettingsKey});

    ESPUI.setInputType(wControlId, "text");

    ESPUI.setElementStyle(wControlId, aElementStyle);

   return wControlId;
}

/**
 * @brief Helper function to add a text control to the web UI with a settings key
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aDefaultText The default text value (default: empty string)
 * @param aHandler The control handler for the control (default: Text)
 *
 * @return The control ID of the newly added text control
 */
Control::ControlId_t WebSite::AddTextControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        SettingsNS::tKey aSettingsKey, const String& aDefaultText, ControlHandler aHandler)
{
    String wValue = aDefaultText;

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wValue = Settings.GetValue<String>(aSettingsKey, aDefaultText);
    }

    Control::ControlId_t wControlId = AddTextControl(apLabel, aParent, aElementStyle, wValue, aSettingsKey, aHandler);

    LOG(LOG_DEBUG, "WebSite::AddTextControl() ControlId %04X, param 0x%08X, value %s, handler %d",
        wControlId, aSettingsKey, wValue.c_str(), static_cast<int>(aHandler));

    return wControlId;
}

/**
 * @brief Helper function to add a color control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aDefaultColor The default color value (default: 0x000000)
 * @param aHandler The control handler for the control (default: Color)
 *
 * @return The control ID of the newly added color control
 */
Control::ControlId_t WebSite::AddColorControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        SettingsNS::tKey aSettingsKey, const uint32_t aDefaultColor, ControlHandler aHandler)
{
    uint32_t wColorParam = aDefaultColor;
    char wHexColor[8] = {0};

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wColorParam = Settings.GetValue<uint32_t>(aSettingsKey, aDefaultColor);
    }

    snprintf(wHexColor, sizeof(wHexColor), "#%06lX",
        static_cast<unsigned long>(wColorParam & 0x00FFFFFF));

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::ColorPickr, apLabel, String(wHexColor), Control::Color::Dark, aParent,
            WebSite::ControlCallback, new ControlParam{aHandler, aSettingsKey});

    ESPUI.setElementStyle(wControlId, aElementStyle);

    LOG(LOG_DEBUG, "WebSite::AddColorControl() Control %04X, param 0x%08X, color %s",
        wControlId, wColorParam, wHexColor);

    return wControlId;
}

/**
 * @brief Helper function to add a time control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aDefaultTime The default time value (default: 0)
 * @param aHandler The control handler for the control (default: Time)
 *
 * @return The control ID of the newly added time control
 */
Control::ControlId_t WebSite::AddTimeControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        SettingsNS::tKey aSettingsKey, const uint32_t aDefaultTime, ControlHandler aHandler)
{
    char wTimeStr[6] = {0};  // Buffer for time string in format HH:MM

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        uint32_t wTimeInt = Settings.GetValue<uint32_t>(aSettingsKey, aDefaultTime);
        DateTimeNS::tDateTime wDateTime = DateTimeNS::DwordToDateTime(wTimeInt);

        /* Convert time to string format HH:MM */
        sprintf(wTimeStr, "%02u:%02u", wDateTime.mTime.mHour, wDateTime.mTime.mMinute);
    }

    Control::ControlId_t wControlId = AddTextControl(apLabel, aParent, aElementStyle, String(wTimeStr), aSettingsKey, aHandler);
    ESPUI.setInputType(wControlId, "time");

    LOG(LOG_DEBUG, "WebSite::AddTimeInput() Control %04X, time %s", wControlId, String(wTimeStr).c_str());

    return wControlId;
}

/**
 * @brief Helper function to add a password control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aHandler The control handler for the control (default: Password)
 *
 * @return The control ID of the newly added password control
 */
Control::ControlId_t WebSite::AddPasswordControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        SettingsNS::tKey aSettingsKey, ControlHandler aHandler)
{
    Control::ControlId_t wControlId = AddTextControl(apLabel, aParent, aElementStyle, emptyString, aSettingsKey, aHandler);
    ESPUI.setInputType(wControlId, "password");

    LOG(LOG_DEBUG, "WebSite::AddPasswordControl() Control %04X", wControlId);

    return wControlId;
}

/**
 * @brief Helper function to add a switcher control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aDefaultState The default state of the switcher (default: false)
 * @param aHandler The control handler for the control (default: Switcher)
 *
 * @return The control ID of the newly added switcher control
 */
Control::ControlId_t WebSite::AddSwitcherControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        SettingsNS::tKey aSettingsKey, const bool aDefaultState, ControlHandler aHandler)
{
    bool wState = aDefaultState;

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wState = Settings.GetValue<bool>(aSettingsKey, aDefaultState);
    }

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Switcher, apLabel, wState, Control::Color::Dark, aParent,
            WebSite::ControlCallback, new ControlParam{aHandler, aSettingsKey});

    ESPUI.setElementStyle(wControlId, aElementStyle);

    LOG(LOG_DEBUG, "WebSite::AddSwitcherControl() Control %04X, default state %s",
        wControlId, aDefaultState ? "ON" : "OFF");

    return wControlId;
}
/**
 * @brief Helper function to add a select control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param apItems The array of items for the select control
 * @param aItemsCount The number of items in the array
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aDefaultOption The default selected option (default: 0)
 * @param aHandler The control handler for the control (default: Select)
 *
 * @return The control ID of the newly added select control
 */
Control::ControlId_t WebSite::AddSelectControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        const char* const* apItems, uint8_t aItemsCount,
        SettingsNS::tKey aSettingsKey, const uint8_t aDefaultOption, ControlHandler aHandler)
{
    uint8_t wSelectedOption = 0;

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Select, apLabel, "", Control::Color::None, aParent,
            WebSite::ControlCallback, new ControlParam{aHandler, aSettingsKey});

    ESPUI.setElementStyle(wControlId, aElementStyle);

    if (apItems != nullptr)
    {
        for (uint8_t wI = 0; wI < aItemsCount; wI++)
        {
            ESPUI.addControl(Control::Type::Option, apItems[wI], static_cast<long>(wI), Control::Color::None, wControlId);
        }
    }

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wSelectedOption = Settings.GetValue<uint8_t>(aSettingsKey, aDefaultOption);
    }

    ESPUI.updateSelect(wControlId, wSelectedOption);

    return wControlId;
}

/**
 * @brief Helper function to add a percentage slider control to the web UI
 * @param apLabel The label for the control
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aSettingsKey The settings key to bind the control to (default: invalid key)
 * @param aDefaultValue The default value of the slider (default: 50)
 * @param aHandler The control handler for the control (default: Slider)
 *
 * @return The control ID of the newly added percentage slider control
 */
Control::ControlId_t WebSite::AddPercentageSliderControl(const char* apLabel, Control::ControlId_t aParent, const char* aElementStyle,
        SettingsNS::tKey aSettingsKey, const uint8_t aDefaultValue, ControlHandler aHandler)
{
    uint8_t wValue = aDefaultValue;

    if (aSettingsKey != ConfigNS::mInvalidKey)
    {
        wValue = Settings.GetValue<uint8_t>(aSettingsKey, aDefaultValue);
    }

    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Slider, apLabel, wValue, Control::Color::Dark, aParent,
            WebSite::ControlCallback, new ControlParam{aHandler, aSettingsKey});

    ESPUI.setElementStyle(wControlId, aElementStyle);

    ESPUI.addControl(Control::Type::Min, "", String(  0), Control::Color::None, wControlId);
    ESPUI.addControl(Control::Type::Max, "", String(100), Control::Color::None, wControlId);

    LOG(LOG_DEBUG, "WebSite::AddPercentageSliderControl() Control %04X, value %d", wControlId, wValue);

    return wControlId;
}

/**
 * @brief Helper function to add a button control to the web UI
 * @param apLabel The label for the control
 * @param arValue The value of the button
 * @param aParent The parent control ID (default: no parent)
 * @param aElementStyle The CSS style for the element (default: "")
 * @param aHandler The control handler for the control (default: None)
 *
 * @return The control ID of the newly added button control
 */
Control::ControlId_t WebSite::AddButtonControl(const char* apLabel, const String& arValue, Control::ControlId_t aParent, const char* aElementStyle,
        ControlHandler aHandler)
{
    Control::ControlId_t wControlId = ESPUI.addControl(Control::Type::Button, apLabel, arValue, Control::Color::None, aParent,
            WebSite::ControlCallback, new ControlParam{aHandler, ConfigNS::mInvalidKey});

    ESPUI.setElementStyle(wControlId, aElementStyle);

    LOG(LOG_DEBUG, "WebSite::AddButtonControl() Control %04X", wControlId);

    return wControlId;
}

/**
 * @brief Build an SVG representation of the display preview
 * @param apDisplayScene Pointer to the display scene
 * @return SVG string representing the display preview
 */
String WebSite::BuildDisplayPreviewSvg(const DisplayScene* apDisplayScene) const
{
    /* Array for cell colors */
    tColor wCellColors[MATRIX_HEIGHT][MATRIX_WIDTH] = {};

    /* Retrieve background color from settings */
    tColor wBackgroundColor = static_cast<tColor>(Settings.GetValue<uint32_t>(
        ConfigNS::mKeyDisplayColorBkgd, ConfigNS::mDefaultDisplayColorBkgd) & COLOR_RGB_MASK);

    /* Check if display scene is available */
    if (apDisplayScene)
    {
        /* Use background color from display scene */
        wBackgroundColor = apDisplayScene->mColorBkgd & COLOR_RGB_MASK;

        /* Fill colors array with background color */
        for (uint8_t wRow = 0; wRow < MATRIX_HEIGHT; ++wRow)
        {
            for (uint8_t wCol = 0; wCol < MATRIX_WIDTH; ++wCol)
            {
                wCellColors[wRow][wCol] = wBackgroundColor;
            }
        }

        /* Lambda to add a word group to the display preview */
        auto wAddGroup = [&wCellColors](const WordGroup& arGroup)
        {
            if (arGroup.IsEnabled())
            {
                tColor wGroupColor = arGroup.mColor & COLOR_RGB_MASK;

                /**
                 * Iterate over the words in the group and fill the corresponding cells
                 *  in the colors array with the group's color.
                 *  Cells not covered by any word keep the background color.
                 */
                for (uint8_t wI = 0; wI < arGroup.mWordsCount; wI++)
                {
                    tWord wWord = arGroup.mWords[wI];

                    if ((wWord > WORD_END_OF_WORDS) &&
                        (wWord < WORD_MAX_NUMBER))
                    {
                        const tWordData& wData = mcWordDataArray[wWord];

                        if ((wData.mRow < MATRIX_HEIGHT) &&
                            (wData.mColumn < MATRIX_WIDTH))
                        {
                            /* Calculate the end column for the word */
                            uint16_t wEndColumn = min(
                                static_cast<uint16_t>(wData.mColumn + wData.mLength),
                                static_cast<uint16_t>(MATRIX_WIDTH));

                            for (uint16_t wCol = wData.mColumn; wCol < wEndColumn; ++wCol)
                            {
                                wCellColors[wData.mRow][wCol] = wGroupColor;
                            }
                        }
                    }
                }
            }
        };

        /* Add word groups to the display preview */
        wAddGroup(apDisplayScene->mItIs);
        wAddGroup(apDisplayScene->mTime);
        wAddGroup(apDisplayScene->mExtraMinutes);
    }

    /**
     * Build SVG for display preview
     */

    /* Reserve 16 KB for the SVG string to avoid frequent reallocations */
    String wSvg;
    wSvg.reserve(16 * 1024);

    /* SVG element buffer */
    char wElement[128];

    /* Initialize SVG string */
    wSvg = "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 160 160' "
           "width='160' height='160' style='display:block;width:100%;height:auto;margin:0 auto'>";
    /* Add styles */
    wSvg += "<style type='text/css'>"
        "text{font-size:7px;font-weight:600;font-family:monospace;stroke-width:0.25;stroke:#404040}"
        ".none{fill:#000000}"
        "</style>";

    /* Add background rectangle in black color */
    wSvg += "<rect width='160' height='160' fill='#000000'/>";

    /* Add letters to the SVG */
    for (uint16_t wRow = 0; wRow < MATRIX_HEIGHT; ++wRow)
    {
        for (uint16_t wCol = 0; wCol < MATRIX_WIDTH; ++wCol)
        {
            /* Extract the letter from the display layout */
            char wLetter = mDisplayLayout[wRow][wCol * 3 + 2];

            if (apDisplayScene)
            {
                /* Format the SVG text element for the letter */
                snprintf(wElement, sizeof(wElement),
                        "<text x='%u' y='%u' fill='#%06lX'>%c</text>",
                        static_cast<unsigned>(wCol * 10 + 7),
                        static_cast<unsigned>(wRow * 10 + 7),
                        static_cast<unsigned long>(wCellColors[wRow][wCol]),
                        wLetter);
            }
            else
            {
                /* Format the SVG text element for the letter with the 'none' class */
                snprintf(wElement, sizeof(wElement),
                        "<text x='%u' y='%u' class='none'>%c</text>",
                        static_cast<unsigned>(wCol * 10 + 7),
                        static_cast<unsigned>(wRow * 10 + 7),
                        wLetter);
            }

            /* Append the formatted SVG element to the SVG string */
            wSvg += wElement;
        }
    }

    /* Close the SVG element */
    wSvg += "</svg>";

    LOG(LOG_DEBUG, "WebSite::BuildDisplayPreviewSvg() SVG length: %u", wSvg.length());

    return wSvg;
}

void WebSite::UpdateDisplayPreview(const MessageNS::Message& arMessage)
{
    DisplayScene wSceneSnapshot;
    mDisplaySceneSnapshot.Get(wSceneSnapshot);

    ESPUI.updateLabel(mWebUIControlID.mDisplayPreview,
        BuildDisplayPreviewSvg(&wSceneSnapshot));
}

void WebSite::UpdateWiFiSettingsControls(bool aForceUpdate)
{
    tSSIDEntry wCurrentSsid;

    /* LOG */
    LOG(LOG_DEBUG, "WebSite::UpdateWiFiSettingsControls() Force update %d", aForceUpdate);

    /* Limit the SSID count to the UI's maximum supported options */
    size_t wCount = mSSIDList.Size();
    wCount = (wCount < mcMaxSsidCount) ? wCount : mcMaxSsidCount;

    const bool wHasSsids = (wCount > 0);

    /* Copy bounded SSID labels locally */
    for (size_t wI = 0; wI < wCount; ++wI)
    {
        tSSIDEntry wSsidEntry{};
        if (!mSSIDList.Get(wI, wSsidEntry))
        {
            wCount = wI;
            break;
        }

        auto& wLabel = mWifiSsidLabels[wI];
        snprintf(wLabel.data(), wLabel.size(), "%s", wSsidEntry.mSsid);

        if ((wI == 0) && !ESPUI.captivePortal)
        {
            wCurrentSsid.Set(wSsidEntry);
        }
    }

    /* Reuse controls and fixed-address labels instead of rebuilding the UI */
    for (size_t wI = 0; wI < wCount; wI++)
    {
        const char* wpLabel = mWifiSsidLabels[wI].data();

        if (wI < mWebUIControlID.mWifiSSIDList.size())
        {
            const auto wControlId = mWebUIControlID.mWifiSSIDList[wI];
            ESPUI.updateControlLabel(wControlId, wpLabel);
            ESPUI.updateControlValue(wControlId, static_cast<long>(wI));
        }
        else
        {
            const auto wControlId = ESPUI.addControl(Control::Type::Option, wpLabel,
                static_cast<long>(wI), Control::Color::None, mWebUIControlID.mWifiSSIDs);
            mWebUIControlID.mWifiSSIDList.push_back(wControlId);
        }
    }

    /* Remove surplus browser options, retaining their controls for later reuse */
    for (size_t wI = wCount; wI < mWebUIControlID.mWifiSSIDList.size(); ++wI)
    {
        ESPUI.updateControlValue(mWebUIControlID.mWifiSSIDList[wI], -2L);
    }

    /* Check if captive portal is enabled */
    if (ESPUI.captivePortal)
    {
        /* WiFi access point (offline) */
        wCurrentSsid.Clear();
    }
    else
    {
        /* Connected to WiFi router */

        if (!wHasSsids)
        {
            /* No SSIDs available, clear selected SSID */
            wCurrentSsid.Clear();

            LOG(LOG_ERROR, "WebSite::UpdateWiFiSettingsControls() Currently used SSID not available, clear selected SSID");
        }
    }

    ESPUI.updateSelect(mWebUIControlID.mWifiSSIDs, wHasSsids ? 0 : -1);
    ESPUI.updateText(mWebUIControlID.mWifiPassword, wCurrentSsid.mPassword);

    /* Restore scan button label */
    ESPUI.updateLabel(mWebUIControlID.mWifiScanButton, "Search for WiFi");

    /* Update forget network button label: "Forget network" or "Forget network <SSID>" if selected SSID is not valid */
    String wLabel = "Forget network";
    if (!ESPUI.captivePortal && wCurrentSsid.IsValid())
    {
        wLabel += " ";
        wLabel += wCurrentSsid.mSsid;
    }
    ESPUI.updateLabel(mWebUIControlID.mWifiForgetButton, wLabel);

    /* Enable/disable controls based on current state */
    ESPUI.setEnabled(mWebUIControlID.mWifiSSIDs,            true);
    ESPUI.setEnabled(mWebUIControlID.mWifiPassword,         true);
    ESPUI.setEnabled(mWebUIControlID.mWifiPasswordShowHide, true);
    ESPUI.setEnabled(mWebUIControlID.mWifiScanButton,       true);
    ESPUI.setEnabled(mWebUIControlID.mWifiConnectButton,    false);
    ESPUI.setEnabled(mWebUIControlID.mWifiForgetButton,     (!ESPUI.captivePortal));   // Only enable forget button if connected to WiFi router

    if (aForceUpdate)
    {
        ESPUI.jsonReload();
    }
}

void WebSite::ControlCallback(BasicControl* apSender, int aType , void* apParam)
{
    /* Check if the website instances and inputs are valid */
    if (mpWebSiteInstance &&                                // Check if the website instance is valid
        mpWebSiteInstance->mpControlEventQueue &&           // Check if the control event queue
        mpWebSiteInstance->mpControlEventNotification &&    //          and control event notification are available
        apSender && apParam)                                // Check if the sender and parameter are valid
    {
        const ControlParam* wParam = static_cast<const ControlParam*>(apParam);
        WebSite* wWebSite = mpWebSiteInstance;

        ControlEvent wEvent{};

        wEvent.mControlId    = apSender->GetId();
        wEvent.mEventType    = aType;
        wEvent.mControlParam = *wParam;

        wEvent.mPayload.mParameter.mValue = static_cast<int32_t>(apSender->getValueInt());

        /* Lambda function to safely copy text into a destination buffer */
        auto wCopyText = [](char* apDestination, size_t aDestinationSize, const String& arText) -> bool
        {
            bool wRetValue = false;

            if (arText.length() < aDestinationSize)
            {
                snprintf(apDestination, aDestinationSize, "%s", arText.c_str());
                wRetValue = true;
            }

            return wRetValue;
        };

        switch (wEvent.mControlParam.mHandler)
        {
            /* For controls that require text or color values, copy the value into the event structure */
            case ControlHandler::Text:
            case ControlHandler::Color:
            case ControlHandler::Timer:
                /* Copy the text value from the sender control into the event structure */
                if (!wCopyText(wEvent.mPayload.mParameter.mValueText,
                        sizeof(wEvent.mPayload.mParameter.mValueText), apSender->getValue()))
                {
                    LOG(LOG_ERROR, "WebSite::ControlCallback() Text value too long for control %04X", wEvent.mControlId);
                    /* Set the handler to None to indicate that the event should not be processed further */
                    wEvent.mControlParam.mHandler = ControlHandler::None;
                }
                break;

            /* For WiFi controls, copy the SSID index and password into the event structure */
            case ControlHandler::WifiSsidSelect:
            case ControlHandler::WifiPassword:
            case ControlHandler::WifiPasswordShowHide:
            case ControlHandler::WifiConnectButton:
            case ControlHandler::WifiScanButton:
            case ControlHandler::WifiForgetButton:
            {
                BasicControl* wSsidControl     = ESPUI.getControl(wWebSite->mWebUIControlID.mWifiSSIDs);
                BasicControl* wPasswordControl = ESPUI.getControl(wWebSite->mWebUIControlID.mWifiPassword);

                if (wSsidControl && wPasswordControl)
                {
                        wEvent.mPayload.mWifi.mSsidIndex = static_cast<int32_t>(wSsidControl->getValueInt());

                        if (!wCopyText(wEvent.mPayload.mWifi.mPassword,
                                sizeof(wEvent.mPayload.mWifi.mPassword), wPasswordControl->getValue()))
                    {
                        LOG(LOG_ERROR, "WebSite::ControlCallback() WiFi password exceeds the supported length");
                        /* Set the handler to None to indicate that the event should not be processed further */
                        wEvent.mControlParam.mHandler = ControlHandler::None;
                    }
                }
                else
                {
                    LOG(LOG_ERROR, "WebSite::ControlCallback() WiFi controls are unavailable");
                    /* Set the handler to None to indicate that the event should not be processed further */
                    wEvent.mControlParam.mHandler = ControlHandler::None;
                }
                break;
            }

            /* Handle other control types that do not require additional data copying */
            case ControlHandler::Switcher:
            case ControlHandler::Select:
            case ControlHandler::Slider:
            default:
                // nothing to do here
                break;
        }

        /* Send the control event to the queue if it has a valid handler */
        if (wEvent.mControlParam.mHandler != ControlHandler::None)
        {
            /* Attempt to send the control event to the FreeRTOS queue and notify the task if successful */
            if (mpWebSiteInstance->mpControlEventQueue->add(wEvent, 0) == pdTRUE)
            {
                mpWebSiteInstance->mpControlEventNotification->Notify();
            }
            else
            {
                LOG(LOG_ERROR, "WebSite::ControlCallback() Control event queue is full; dropping control %04X", wEvent.mControlId);
            }
        }
    }
}
