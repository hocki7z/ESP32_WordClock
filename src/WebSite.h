/*
 * WebSite.h
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#ifndef WEBSITE_H_
#define WEBSITE_H_

#include <Arduino.h>
#include <ESPUI.h>

#include "Application.h"
#include "Configuration.h"


class WebSite : public ApplicationNS::Task
{
public:
    WebSite(char const* apName, ApplicationNS::tTaskPriority aPriority, const uint32_t aStackSize);
    virtual ~WebSite();

    /* ApplicationNS::Task::Init */
    void Init(ApplicationNS::tTaskObjects* apTaskObjects) override;

private:

    struct tWebUIControlID
    {
        Control::ControlId_t mDisplayClockMode;
        Control::ControlId_t mDisplayClockItIs;
        Control::ControlId_t mDisplayClockSingleMinutes;
        Control::ControlId_t mDisplayColorTime;
        Control::ControlId_t mDisplayColorBackground;

        Control::ControlId_t mDisplayLedBrightness;

        Control::ControlId_t mDisplayUseNightMode;
        Control::ControlId_t mDisplayBrightnessNightMode;
        Control::ControlId_t mDisplayNightModeStartTime;
        Control::ControlId_t mDisplayNightModeEndTime;

        Control::ControlId_t mDatetimeNtpServer;
        Control::ControlId_t mDatetimeTimeZone;

        Control::ControlId_t mWifiSSIDs;
        Control::ControlId_t mWifiPassword;
        Control::ControlId_t mWifiPasswordShowHide;

        std::vector<Control::ControlId_t> mWifiSSIDList;

        Control::ControlId_t mWifiScanButton;
        Control::ControlId_t mWifiConnectButton;

    };

    /** @brief Type definition for control callback function pointer
     *       @param apControl Pointer to the control that triggered the callback
     *       @param aType Type of the event
     *       @param apParam Additional parameter passed to the callback
     */
    using ControlCallback_t = void(*)(BasicControl*, int, void*);
    

    /** @brief "This" pointer for created WebSite instance */
    static WebSite* mpWebSiteInstance;

    tWebUIControlID mWebUIControlID;

    std::vector<ConfigNS::tSSIDEntry> mLocalSsidList;


    /* ApplicationNS::Task::ProcessIncomingMessage() */
    void ProcessIncomingMessage(const MessageNS::Message &arMessage) override;

    void HandleControl(BasicControl* apControl, int aType, void* apParam = nullptr);

    void HandleColorControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey);
    void HandleSwitcherControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey);
    void HandleSelectControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey);
    void HandlePercentageSliderControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey);
    void HandleTimerControl(BasicControl* aControl, int aType, SettingsNS::tKey aSettingsKey);

    Control::ControlId_t AddTextControl(const char* apTitle, Control::ControlId_t aParent = Control::noParent, const String& aElementStyle = "",
        const String& arValue = emptyString);

    Control::ControlId_t AddTextControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const String& aElementStyle = "",
        SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, const String& aDefaultText = emptyString);

    Control::ControlId_t AddColorControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const String& aElementStyle = "",
        SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, const uint32_t aDefaultColor = 0x000000);

    Control::ControlId_t AddTimeControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const String& aElementStyle = "",
        SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, const uint32_t aDefaultTime = 0);

    Control::ControlId_t AddPasswordControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const String& aElementStyle = "");

    Control::ControlId_t AddSwitcherControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const String& aElementStyle = "",
        SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, const bool aDefaultState = false);

    Control::ControlId_t AddSelectControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const String& aElementStyle = "",
        const char* const* apItems = nullptr, uint8_t aItemsCount = 0,
        SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, const uint8_t aDefaultOption = 0);

    Control::ControlId_t AddPercentageSliderControl(const char* apLabel, Control::ControlId_t aParent = Control::noParent, const String& aElementStyle = "",
        SettingsNS::tKey aSettingsKey = ConfigNS::mInvalidKey, const uint8_t aDefaultValue = 50);

    Control::ControlId_t AddButtonControl(const char* apLabel, const String& arValue, Control::ControlId_t aParent = Control::noParent, const String& aElementStyle = "");

    void UpdateLedBrightnessControls(bool aForceUpdate = false);

    void UpdateWiFiSettingsControls(bool aForceUpdate = false);

    static void ControlCallback(BasicControl* apSender, int aType, void* apParam = nullptr);
};

#endif /* WEBSITE_H_ */