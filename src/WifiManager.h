/*
 * WiFiManager.h
 *
 *  Created on: 13.09.2025
 *      Author: hocki
 */
#pragma once

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
        STATE_STA_CONNECTING,
        STATE_STA_CONNECTED,
//        STATE_STA_DISCONNECTED,
        STATE_AP_STARTING,
        STATE_AP_STARTED,
//        STATE_AP_STOPPED,
//        STATE_OFF,
    } tState;

    /* Frequent timer used to service captive-portal DNS while the AP is active. */
    ApplicationNS::tTaskTimerObjects mDnsTimerObjects;
    ApplicationNS::TaskTimer* mpDnsTimer = nullptr;

    /** @brief DNS server instance */
    DNSServer mDnsServer;
    /** @brief Current WiFi manager status */
    tState mState  = STATE_IDLE;

    /* Timer ID for DNS timer */
    static constexpr uint32_t mcDnsTimerId = 0x02;
    /* Ensure that the DNS timer ID does not conflict with the task timer ID */
    static_assert(mcDnsTimerId != ApplicationNS::Task::mcTaskTimerId, "DNS timer ID conflicts with task timer ID");

    /** @brief Timeout used for WiFi STA connection */
    static constexpr uint32_t mConnectionTimeout = 30000U;  // 30 seconds

    /* Number of maximum connection attempts */
    static constexpr uint8_t mMaxConnectionAttempts = 3;

    /** @brief Number of connection attempts */
    uint8_t mConnectionAttempts = 0;

    /** @brief Start time of connection attempt */
    uint32_t mConnectionStart = 0;

    /** @brief Flag to indicate if WiFi scan has been run once */
    bool mWifiScanRunOnce = false;

    /* Override ApplicationNS::Task::ProcessTimerEvent() */
    void ProcessTimerEvent(const uint32_t aTimerId = 0) override;

    /* Override ApplicationNS::Task::ProcessIncomingMessage() */
    void ProcessIncomingMessage(const MessageNS::Message &arMessage) override;

    void ProcessState(const WiFiEvent_t aEvent = ARDUINO_EVENT_MAX);

    void UpdateSSIDList(bool aCheckScanResults);

    void SendMessage(MessageNS::tMessageId wMessageId);

    bool IsInternetAvailable(void);

    bool ConnectWifi(void);

    void ConnectAP(void);

    void HandleWifiEvent(WiFiEvent_t aEvent);
};

