/*
 * WebSite.h
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>
#include <array>
#include <type_traits>

#include <ESPUI.h>

#include "Application.h"
#include "Configuration.h"
#include "Settings.hpp"


class WebSite : public ApplicationNS::Task
{
public:
    WebSite(char const* apName, ApplicationNS::tTaskPriority aPriority, const uint32_t aStackSize);
    virtual ~WebSite();

    /* ApplicationNS::Task::Init */
    void Init(ApplicationNS::tTaskObjects* apTaskObjects) override;

private:

    /** @brief Tag identifying which Handle*Control() function should process a control's change, carried via ESPUI's void* callback param */
    enum class ControlHandler : uint8_t
    {
        None = 0,

        /* Handles for parameters */
        Text,
        Color,
        Timer,
        Switcher,
        Select,
        Slider,

        /* Special handlers for WiFi settings */
        WifiSsidSelect,
        WifiPassword,
        WifiPasswordShowHide,
        WifiConnectButton,
        WifiScanButton,
        WifiForgetButton,
    };

    /** @brief Parameters associated with a control (used as the void* callback param in ESPUI) */
    struct ControlParam
    {
        ControlHandler mHandler = ControlHandler::None;
        SettingsNS::tKey mSettingsKey = ConfigNS::mInvalidKey;
    };

    /** @brief Value snapshot copied from ESPUI's asynchronous callback into the WebSite task queue */
    struct ControlEvent
    {
        /* Control event metadata */
        Control::ControlId_t mControlId = Control::noParent;
        int mEventType = 0;
        ControlParam mControlParam{};

        /** @brief Exactly one payload is active, selected by mHandler. */
        union
        {
            struct
            {
                int32_t mValue;
                char mValueText[SSID_PASSWORD_SIZE];
            } mParameter;

            struct
            {
                int32_t mSsidIndex;
                char mPassword[SSID_PASSWORD_SIZE];
            } mWifi;
        } mPayload{};
    };

    /** @brief Ensure ControlEvent remains trivially copyable */
    static_assert(std::is_trivially_copyable<ControlEvent>::value,
        "ControlEvent must remain byte-copyable for the FreeRTOS queue");

    /** @brief Task notification value for control events */
    static constexpr uint32_t mTaskNotificationControlEvent = 0x82;     //binary: 00000000 00000000 00000000 10000010
    /** @brief Length of the control event queue */
    static constexpr UBaseType_t   mControlEventQueueLength = 16;

    /** @brief Type alias for the control event queue */
    using ControlEventQueue = FreeRTOScpp::Queue<ControlEvent, mControlEventQueueLength>;

    struct tWebUIControlID
    {
        /* Top-level tabs */
        Control::ControlId_t mOverViewTab;
        Control::ControlId_t mLedDisplayTab;
        Control::ControlId_t mSettingsTab;

        /* Overview tab controls */
        Control::ControlId_t mPreviewGroup;
        Control::ControlId_t mDisplayPreview;

        /* LED Display tab controls */
        Control::ControlId_t mLedDisplayDisplayGroup;
        Control::ControlId_t mDisplayClockMode;
        Control::ControlId_t mDisplayClockItIs;
        Control::ControlId_t mDisplayClockSingleMinutes;
        //
        Control::ControlId_t mLedDisplayColorGroup;
        Control::ControlId_t mDisplayColorTime;
        Control::ControlId_t mDisplayColorBkgd;
        Control::ControlId_t mDisplayColorTimeNight;
        Control::ControlId_t mDisplayColorBkgdNight;
        //
        Control::ControlId_t mLedDisplayBrightnessGroup;
        Control::ControlId_t mDisplayUseDayNightMode;
        Control::ControlId_t mDisplayDayModeStartTime;
        Control::ControlId_t mDisplayDayModeEndTime;
        Control::ControlId_t mDisplayBrightnessTime;
        Control::ControlId_t mDisplayBrightnessBkgd;
        Control::ControlId_t mDisplayBrightnessTimeNight;
        Control::ControlId_t mDisplayBrightnessBkgdNight;

        /* Settings tab controls */
        Control::ControlId_t mSettingsTimeGroup;
        Control::ControlId_t mDatetimeNtpServer;
        Control::ControlId_t mDatetimeTimeZone;
        //
        Control::ControlId_t mSettingsWiFiGroup;
        Control::ControlId_t mWifiSSIDs;
        Control::ControlId_t mWifiPassword;
        Control::ControlId_t mWifiPasswordShowHide;
        //
        std::vector<Control::ControlId_t> mWifiSSIDList;
        //
        Control::ControlId_t mWifiScanButton;
        Control::ControlId_t mWifiConnectButton;
        Control::ControlId_t mWifiForgetButton;
    };

    /** @brief "This" pointer for created WebSite instance */
    static WebSite* mpWebSiteInstance;

    /** @brief Web UI control IDs */
    tWebUIControlID mWebUIControlID;

    /** @brief Bounded option pool; labels must outlive ESPUI controls. */
    static constexpr size_t mcMaxSsidCount = 64;

    /** @brief SSID labels for the bounded option pool */
    std::array<std::array<char, SSID_NAME_SIZE>, mcMaxSsidCount> mWifiSsidLabels{};

    /** @brief Pointers to the control event queue and its notification */
    ControlEventQueue* mpControlEventQueue = nullptr;
    TaskNotification*  mpControlEventNotification = nullptr;

    /* ApplicationNS::Task::ProcessIncomingMessage() */
    void ProcessIncomingMessage(const MessageNS::Message &arMessage) override;
    /* ApplicationNS::Task::ProcessUnknownNotification() */
    void ProcessUnknownNotification(uint32_t aNotificationValue) override;

    void HandleControlEvent(const ControlEvent& arEvent);
    void HandleWiFiControlEvent(const ControlEvent& arEvent);

    Control::ControlId_t AddGroupHelper(const char * apLabel, Control::ControlId_t aParent = Control::noParent, Control::Color aColor = Control::Color::None,
        const bool aExpandable = false, const bool aExpanded = false);

    Control::ControlId_t AddLabelControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const char* aElementStyle = "",
        const String& arValue = emptyString);

    Control::ControlId_t AddSeparator(Control::ControlId_t aParent, bool aDrawLine = false);

    Control::ControlId_t AddTextControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const char* aElementStyle = "",
        const String& arValue = emptyString, SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, ControlHandler aHandler = ControlHandler::None);

    Control::ControlId_t AddTextControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const char* aElementStyle = "",
        SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, const String& aDefaultText = emptyString, ControlHandler aHandler = ControlHandler::Text);

    Control::ControlId_t AddColorControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const char* aElementStyle = "",
        SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, const uint32_t aDefaultColor = 0x000000, ControlHandler aHandler = ControlHandler::Color);

    Control::ControlId_t AddTimeControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const char* aElementStyle = "",
        SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, const uint32_t aDefaultTime = 0, ControlHandler aHandler = ControlHandler::Timer);

    Control::ControlId_t AddPasswordControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const char* aElementStyle = "",
        SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, ControlHandler aHandler = ControlHandler::Text);

    Control::ControlId_t AddSwitcherControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const char* aElementStyle = "",
        SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, const bool aDefaultState = false, ControlHandler aHandler = ControlHandler::Switcher);

    Control::ControlId_t AddSelectControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const char* aElementStyle = "",
        const char* const* apItems = nullptr, uint8_t aItemsCount = 0,
        SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, const uint8_t aDefaultOption = 0, ControlHandler aHandler = ControlHandler::Select);

    Control::ControlId_t AddPercentageSliderControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const char* aElementStyle = "",
        SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, const uint8_t aDefaultValue = 50, ControlHandler aHandler = ControlHandler::Slider);

    Control::ControlId_t AddButtonControl(const char* apLabel, const String& arValue, Control::ControlId_t aParent = Control::noParent, const char* aElementStyle = "",
        ControlHandler aHandler = ControlHandler::None);


    String BuildDisplayPreviewSvg(const DisplayScene* apDisplayScene = nullptr) const;

    void UpdateDisplayPreview(const MessageNS::Message& arMessage);
    void UpdateWiFiSettingsControls(bool aForceUpdate = false);

    static void ControlCallback(BasicControl* apSender, int aType, void* apParam = nullptr);
};
