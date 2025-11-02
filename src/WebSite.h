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
    };

    /** @brief "This" pointer for created WebSite instance */
    static WebSite* mpWebSiteInstance;

    tWebUIControlID mWebUIControlID;

    /* ApplicationNS::Task::ProcessIncomingMessage() */
    void ProcessIncomingMessage(const MessageNS::Message &arMessage) override;

    void HandleControl(Control* apControl, int aType);
    void HandleColorControl(Control* aControl, int aType, SettingsNS::tKey aSettingsKey);
    void HandleSwitcherControl(Control* aControl, int aType, SettingsNS::tKey aSettingsKey);
    void HandleSelectControl(Control* aControl, int aType, SettingsNS::tKey aSettingsKey);
    void HandlePercentageSliderControl(Control* aControl, int aType, SettingsNS::tKey aSettingsKey);
    void HandleTimerControl(Control* aControl, int aType, SettingsNS::tKey aSettingsKey);

    Control::ControlId_t AddColorControl(const char* apTitle, SettingsNS::tKey aSettingsKey, const uint32_t aDefaultColor = 0x000000);
    Control::ControlId_t AddSwitcherControl(const char* apTitle, SettingsNS::tKey aSettingsKey, const bool aDefaultState = false);
    Control::ControlId_t AddSelectControl(const char* apTitle, const char* const* apItems, uint8_t aItemsCount, SettingsNS::tKey aSettingsKey, const uint8_t aDefaultOption = 0);
    Control::ControlId_t AddPercentageSliderControl(const char* apTitle, SettingsNS::tKey aSettingsKey, const uint8_t aDefaultValue = 50);
    Control::ControlId_t AddTimeControl(const char* apTitle, SettingsNS::tKey aSettingsKey, const uint32_t aDefaultTime = 0);

    void UpdateLedBrightnessControls(bool aForceUpdate = false);

    static void ControlCallback(Control* apSender, int aType);

};

#endif /* WEBSITE_H_ */