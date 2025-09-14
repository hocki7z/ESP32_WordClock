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


class WiFiManager
{
public:
	typedef enum tStatus
	{
		STATUS_NOT_CONNECTED,
		STATUS_CONNECTING,
		STATUS_ONLINE,
		STATUS_AP_MODE,
	} tStatus;

	WiFiManager();
	virtual ~WiFiManager();

	void Init(void);

	void Loop(void);

	tStatus GetStatus(void);


private:

	typedef enum tState
	{
		STATE_BOOT,
		STATE_NOT_CONFIGURED,
		STATE_AP_MODE,
		STATE_CONNECTING,
		STATE_RECONNECTING,
		STATE_ONLINE,
	} tState;

	uint32_t mPrevMillis = 0;

    tState  mState  = STATE_BOOT;
    tStatus mStatus = STATUS_NOT_CONNECTED;

	/* Timeout used for WiFi STA connection */
	static constexpr uint32_t mConnectionTimeout = 30000U; //30seconds
    uint32_t mConnectionStart = 0;

    /* Last triggered WiFi event */
	WiFiEvent_t mWifiEvent = ARDUINO_EVENT_MAX;
    /* True if a time even has been triggered */
	boolean mWifiEventTriggered = false;


    void ProcessState(void);

    void ConnectWifi(void);
	void ReconnectWifi(void);

    void HandleWifiEvent(WiFiEvent_t aEvent);
};

#endif /* WIFI_MANAGER_H_ */