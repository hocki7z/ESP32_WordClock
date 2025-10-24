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

    Control::ControlId_t AddColorControl(const char* apTitle, SettingsNS::tKey aSettingsKey, const uint32_t aDefaultColor = 0x000000);
    Control::ControlId_t AddSwitcherControl(const char* apTitle, SettingsNS::tKey aSettingsKey, const bool aDefaultState = false);
    Control::ControlId_t AddSelectControl(const char* apTitle, const char* const* apItems, uint8_t aItemsCount, SettingsNS::tKey aSettingsKey,
            const uint8_t aDefaultOption = 0);

    static void ControlCallback(Control* apSender, int aType);

};

#endif /* WEBSITE_H_ */