/*
 * WebSite.h
 *
 *  Created on: 25.09.2025
 *      Author: hocki
 */
#ifndef WEBSITE_H_
#define WEBSITE_H_

#include <Arduino.h>

#include <WiFiGeneric.h>
#include <WiFiType.h>

#include <ESPUI.h>


class WebSite
{
public:
    WebSite();
    virtual ~WebSite();

    void Init(void);

    void Loop(void);

private:
    /* Pointer to this class */
    static WebSite* mpWebSite;

    /* Buffer to create table content */
    char mElementBuffer[5000]={'\0'};
    /* Ttemp string */
    char mElementString[500]={'\0'};

    uint16_t AddGroupHelper(const char * apTitle, Control::Color aColor, uint16_t aTab);
    uint16_t AddGroupValueHelper(const char * apTitle, String aValue, String aUnit, uint16_t aGroup);
    uint16_t AddTextInputHelper(const char * apTitle, uint16_t aGroup);
    uint16_t AddNumberInputHelper(const char * apTitle, uint16_t aGroup);
    uint16_t AddSwitcherInputHelper(const char * apTitle, uint16_t aGroup);

    const char* OnOffString(uint8_t aValue);
    const char* ErrOkString(uint8_t aValue);

    void AddElement(const char* apLabel, const char* apValue);
    void AddElementUnit(const char* apLabel, const char* apValue, const char* apUnit);
    void AddElementWide(const char* apLabel, const char* apValue);
    uint16_t AddEmtyElement(const uint16_t aParent);
    const char* AddUnit(const char* apInput, const char* apUnit);
    void UpdateElements(uint16_t aParent);


    void HandleWifiEvent(WiFiEvent_t aEvent);
};

#endif /* WEBSITE_H_ */