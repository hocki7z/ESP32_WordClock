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

	typedef enum tState
	{
		STATE_BOOT,
		STATE_NOT_CONFIGURED,
		STATE_AP_MODE,
		STATE_CONNECTING,
		STATE_RECONNECTING,
		STATE_ONLINE,
	} tState;

	/* Periodical timer for this task */
	ApplicationNS::tTaskTimerObjects mTimerObjects;
    ApplicationNS::TaskTimer* mpTimer;

    tState  mState  = STATE_BOOT;
    tStatus mStatus = STATUS_NOT_CONNECTED;

	/* Timeout used for WiFi STA connection */
	static constexpr uint32_t mConnectionTimeout = 30000U; //30seconds
    uint32_t mConnectionStart = 0;

	/* ApplicationNS::Task::task() */
    void task(void) override;
    /* ApplicationNS::Task::ProcessTimerEvent() */
    void ProcessTimerEvent(const uint32_t aTimerId = 0) override;

    /* ApplicationNS::Task::ProcessIncomingMessage() */
    void ProcessIncomingMessage(const MessageNS::Message &arMessage) override;

    void ProcessState(const WiFiEvent_t aEvent = ARDUINO_EVENT_MAX);

	void SendStatus(void);

    void ConnectWifi(void);
	void ReconnectWifi(void);

    void HandleWifiEvent(WiFiEvent_t aEvent);
};

#endif /* WIFI_MANAGER_H_ */