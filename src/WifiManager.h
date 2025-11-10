/*
 * WiFiManager.h
 *
 *  Created on: 13.09.2025
 *      Author: hocki
 */
#ifndef WIFI_MANAGER_H_
#define WIFI_MANAGER_H_

#include <Arduino.h>

#include <WiFiGeneric.h>
#include <WiFiType.h>

#include <DNSServer.h>

#include "Application.h"


class WiFiManager : public ApplicationNS::Task
{
public:
    WiFiManager(char const* apName, ApplicationNS::tTaskPriority aPriority, const uint32_t aStackSize);
    virtual ~WiFiManager();

    /* ApplicationNS::Task::Init */
    void Init(ApplicationNS::tTaskObjects* apTaskObjects) override;

private:

    typedef enum tStatus
    {
        STATUS_NOT_CONNECTED,
        STATUS_CONNECTING,
        STATUS_ONLINE,
        STATUS_AP_MODE,
    } tStatus;

    /** @brief WiFi manager state machine states */
    typedef enum tState
    {
        STATE_IDLE,
        STATE_CONNECTING,
        STATE_RECONNECTING,
        STATE_STA_CONNECTED,
//        STATE_STA_DISCONNECTED,
        STATE_AP_STARTED,
//        STATE_AP_STOPPED,
//        STATE_OFF,
    } tState;

    /* Periodical timer for this task */
    ApplicationNS::tTaskTimerObjects mTimerObjects;
    ApplicationNS::TaskTimer* mpTimer;

    /** @brief DNS server instance */
    DNSServer mDnsServer;
    /** @brief Current WiFi manager status */
    tState mState  = STATE_IDLE;

    /** @brief Timeout used for WiFi STA connection */
    static constexpr uint32_t mConnectionTimeout = 30000U;  // 30 seconds
    /** @brief Start time of connection attempt */
    uint32_t mConnectionStart = 0;

    /* ApplicationNS::Task::task() */
    void task(void) override;
    /* ApplicationNS::Task::ProcessTimerEvent() */
    void ProcessTimerEvent(const uint32_t aTimerId = 0) override;

    /* ApplicationNS::Task::ProcessIncomingMessage() */
    void ProcessIncomingMessage(const MessageNS::Message &arMessage) override;

    void ProcessState(const WiFiEvent_t aEvent = ARDUINO_EVENT_MAX);

    void SendMessage(MessageNS::tMessageId wMessageId);

    bool IsWifiModePossible(void);

    bool IsInternetAvailable(void);

    void ConnectWifi(void);

    void ConnectAP(void);

    void HandleWifiEvent(WiFiEvent_t aEvent);
};

#endif /* WIFI_MANAGER_H_ */