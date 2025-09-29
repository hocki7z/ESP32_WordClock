/*
 * WiFiManager.cpp
 *
 *  Created on: 13.09.2025
 *      Author: hocki
 */

/* Start Wifi connection with defined SSID and password */
#define USE_CREDENTIALS

#ifdef USE_CREDENTIALS
/**
    A template for wifi credential file credentials.h in the project root:
    -----------------------------------
        credentials.h
            #ifndef CREDENTIALS_H_
            #define CREDENTIALS_H_

            #define CRED_WIFI_SSID       "MyWiFiSSID"	// SSID of access point
            #define CRED_WIFI_PASS       "MyWiFiPass"   // Password of access point

            #endif
    -----------------------------------
*/
#include "../credentials.h"
#endif /* USE_CREDENTIALS */

#include <WiFi.h>

#include "Logger.h"
#include "WiFiManager.h"


/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)

/* Delay in msec between updates */
constexpr uint32_t mcUpdateDelay = 100;

/**
 * @brief Constructor
 */
WiFiManager::WiFiManager()
{
}

/**
 * @brief Destructor
 */
WiFiManager::~WiFiManager()
{
}

void WiFiManager::Init(void)
{
    /* Register WiFi events listener */
	WiFi.onEvent(
        std::bind(&WiFiManager::HandleWifiEvent, this, std::placeholders::_1));
}

void WiFiManager::Loop(void)
{
	/* Yield should not be necessary, but cannot hurt eather */
  	yield();

    /* Get current system tick */
    uint32_t wCurrMillis = millis();

    /* Check time ticks delta */
    if ((wCurrMillis - mPrevMillis) >= mcUpdateDelay)
    {
        /* Update previous time tick */
        mPrevMillis = wCurrMillis;

		/* Process current FSM state */
		ProcessState();
	}
}

WiFiManager::tStatus WiFiManager::GetStatus(void)
{
    return mStatus;
}

void WiFiManager::ProcessState(void)
{
	switch (mState)
	{
		case STATE_BOOT:
			mStatus = STATUS_NOT_CONNECTED;

#ifdef USE_CREDENTIALS
            /* Try to connect to WiFi */
            mState = STATE_CONNECTING;
            ConnectWifi();
#endif /* USE_CREDENTIALS */

			break;

		case STATE_CONNECTING:
		case STATE_RECONNECTING:
			if (mWifiEventTriggered)
			{
				switch (mWifiEvent)
				{
					case ARDUINO_EVENT_WIFI_STA_CONNECTED:
						mState  = STATE_ONLINE;
						mStatus = STATUS_ONLINE;
						break;

					default:
						break;
				};
			}
			else if (((millis() - mConnectionStart) >= mConnectionTimeout) &&
					 (WiFi.status() != WL_CONNECTED))
			{
				LOG(LOG_ERROR, "WiFiManager::ProcessState() Failed to connect after %d millis", mConnectionTimeout);

				mState  = STATE_RECONNECTING;
				mStatus = STATUS_NOT_CONNECTED;

				/* Try to reconnect to WiFi */
				ReconnectWifi();
			}
			break;

		case STATE_ONLINE:
			if (mWifiEventTriggered)
			{
				switch (mWifiEvent)
				{
					case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
						LOG(LOG_DEBUG, "WiFiManager::ProcessState() Event SYSTEM_EVENT_STA_DISCONNECTED, reconnecting");

						mState  = STATE_RECONNECTING;
						mStatus = STATUS_NOT_CONNECTED;

						ReconnectWifi();
						break;

					default:
						break;
				};
			}
			break;

		case STATE_NOT_CONFIGURED:
		case STATE_AP_MODE:
		default:
			break;
	}

	/* Reset event */
    mWifiEvent = ARDUINO_EVENT_MAX;
    mWifiEventTriggered = false;
}

void WiFiManager::ConnectWifi(void)
{
    /* Disconnect from the network (close AP) */
    WiFi.softAPdisconnect(true);
    /* Disconnect from the network */
    WiFi.disconnect(true);
    delay(100);

    /* FIX problem:
     *    unable to connect to the router
     *    if previous connection failed
     *    (the bugfix seems to work)
     */
    /* Disable persistent for ESP32 */
    WiFi.persistent(false);

    /* Start WiFi Station mode */
    WiFi.mode(WIFI_STA);
    delay(100);

    /* Ste connection start time */
    mConnectionStart = millis();

#ifdef USE_CREDENTIALS
    /* LOG */
    LOG(LOG_DEBUG, "WiFiManager::ConnectWifi() Start WiFi Station mode, credentials SSID: %s", CRED_WIFI_SSID);
    /* Start Wifi connection */
    WiFi.begin(CRED_WIFI_SSID, CRED_WIFI_PASS);
#else
    /* Connect to SDK config */
    LOG(LOG_DEBUG, "WiFiManager::ConnectWifi() Start WiFi Station mode, connect to SDK config");
    WiFi.begin();
#endif /* ifdef USE_CREDENTIALS */
}

void WiFiManager::ReconnectWifi(void)
{
	mConnectionStart = millis();
	WiFi.reconnect();
}

void WiFiManager::HandleWifiEvent(WiFiEvent_t aEvent)
{
    mWifiEventTriggered = true;
    mWifiEvent = aEvent;

    LOG(LOG_VERBOSE, "WiFiManager::HandleWifiEvent() Event: %d", aEvent);

	switch (aEvent)
	{
		case ARDUINO_EVENT_WIFI_STA_START:
			LOG(LOG_VERBOSE, "WiFiManager::HandleWifiEvent() Station start");
			break;

		case ARDUINO_EVENT_WIFI_STA_STOP:
			LOG(LOG_VERBOSE, "WiFiManager::HandleWifiEvent() Station stop");
			break;

		case ARDUINO_EVENT_WIFI_STA_CONNECTED:
			LOG(LOG_DEBUG, "WiFiManager::HandleWifiEvent() Station connected to AP; We are online after %d millis", (millis() - mConnectionStart));
			break;

		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
//TODO there are to much disconnection events if not possible connect to STA with saved SSID isn't exist. FIX IT
			LOG(LOG_DEBUG, "WiFiManager::HandleWifiEvent() Station disconnected from AP");
			break;

		case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
			LOG(LOG_VERBOSE, "WiFiManager::HandleWifiEvent() Auth mode of AP connected by ESP32 station changed");
			break;
		case ARDUINO_EVENT_WIFI_STA_GOT_IP:
			LOG(LOG_DEBUG, "WiFiManager::HandleWifiEvent() Station got IP from connected AP %s", WiFi.localIP().toString().c_str());
			break;
		case ARDUINO_EVENT_WIFI_STA_LOST_IP:
			LOG(LOG_DEBUG, "WiFiManager::HandleWifiEvent() Station lost IP and the IP is reset to 0");
			break;
		default:
			break;
    }
}