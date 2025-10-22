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
#include "Serialize.h"

#include "WiFiManager.h"


/* Log level for this module */
#define LOG_LEVEL   (LOG_DEBUG)

/* Periodical task timer ID */
static constexpr uint32_t mPeriodicalTaskTimerId = 0x01;


/**
 * @brief Constructor
 */
WiFiManager::WiFiManager(char const* apName, ApplicationNS::tTaskPriority aPriority, const uint32_t aStackSize)
    : ApplicationNS::Task(apName, aPriority, aStackSize)
{
    // do nothing
}

/**
 * @brief Destructor
 */
WiFiManager::~WiFiManager()
{
	/* Clean up task timer */
    if (mpTimer)
    {
		/* Stop running timer */
        mpTimer->stop();
		/*    and destroy it */
        delete mpTimer;
        mpTimer = nullptr;
    }
}

void WiFiManager::Init(ApplicationNS::tTaskObjects* apTaskObjects)
{
    /* Initialize base class */
    ApplicationNS::Task::Init(apTaskObjects);

    /* Create periodical timer */
	mTimerObjects.mTaskHandle = this->getTaskHandle();
	mTimerObjects.mpTaskMessagesQueue = this->mpTaskObjects->mpMessageQueue;

    mpTimer = new ApplicationNS::TaskTimer(mPeriodicalTaskTimerId, 1000, true); // 1 sec period
	mpTimer->Init(&mTimerObjects);

    /* Register WiFi events listener */
	WiFi.onEvent(
        std::bind(&WiFiManager::HandleWifiEvent, this, std::placeholders::_1));

	LOG(LOG_VERBOSE, "WiFiManager::Init()");
}

void WiFiManager::task(void)
{
	LOG(LOG_VERBOSE, "WiFiManager::task()");

    /* Start notification timer for this task */
    mpTimer->start();

    /* Execute base class task */
    ApplicationNS::Task::task();
}

void WiFiManager::ProcessTimerEvent(const uint32_t aTimerId)
{
	LOG(LOG_VERBOSE, "WiFiManager::ProcessTimerEvent() Timer ID: %d", aTimerId);

	if (aTimerId == mPeriodicalTaskTimerId)
	{
		/* Process current FSM state */
		ProcessState();
	}
}

void WiFiManager::ProcessIncomingMessage(const MessageNS::Message &arMessage)
{
	LOG(LOG_VERBOSE, "WiFiManager::ProcessIncomingMessage()");

	switch (arMessage.mId)
	{
		case MessageNS::tMessageId::MGS_EVENT_WIFI_EVENT_TRIGGERED:
		{
			/* Deserialize wifi event */
			uint8_t wEvent;
			if (SerializeNS::DeserializeData(arMessage.mPayload, &wEvent) == sizeof(wEvent))
			{
				/* Process WiFi event */
				ProcessState(static_cast<WiFiEvent_t>(wEvent));
			}
		}
			break;

		default:
			// do nothing
			break;
	}
}

void WiFiManager::ProcessState(const WiFiEvent_t aEvent)
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
			switch (aEvent)
			{
				case ARDUINO_EVENT_WIFI_STA_CONNECTED:
					LOG(LOG_DEBUG, "WiFiManager::ProcessState() We are online");

					mState  = STATE_ONLINE;
					mStatus = STATUS_ONLINE;
					/* Notify */
					SendStatus();
					break;

				default:
					/* Check for connection timeout */
					if (((millis() - mConnectionStart) >= mConnectionTimeout) &&
					    (WiFi.status() != WL_CONNECTED))
					{
						LOG(LOG_ERROR, "WiFiManager::ProcessState() Failed to connect after %d millis", mConnectionTimeout);

						mState  = STATE_RECONNECTING;
						mStatus = STATUS_NOT_CONNECTED;

						/* Notify */
						SendStatus();

						/* Try to reconnect to WiFi */
						ReconnectWifi();
					}
					break;
			};
			break;

		case STATE_ONLINE:
			switch (aEvent)
			{
				case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
					LOG(LOG_DEBUG, "WiFiManager::ProcessState() Event SYSTEM_EVENT_STA_DISCONNECTED, reconnecting");

					mState  = STATE_RECONNECTING;
					mStatus = STATUS_NOT_CONNECTED;

					/* Notify */
					SendStatus();

					/* Try to reconnect to WiFi */
					ReconnectWifi();
					break;

				default:
					break;
			};
			break;

		case STATE_NOT_CONFIGURED:
		case STATE_AP_MODE:
		default:
			break;
	}
}

void WiFiManager::SendStatus(void)
{
    MessageNS::tMessageId wMessageId = MessageNS::tMessageId::NONE;

    switch (mStatus)
    {
        case STATUS_NOT_CONNECTED:
            wMessageId = MessageNS::tMessageId::MGS_STATUS_WIFI_NOT_CONNECTED;
            break;
        case STATUS_CONNECTING:
            wMessageId = MessageNS::tMessageId::MGS_STATUS_WIFI_CONNECTING;
            break;
        case STATUS_ONLINE:
            wMessageId = MessageNS::tMessageId::MGS_STATUS_WIFI_STA_CONNECTED;
            break;
        case STATUS_AP_MODE:
            wMessageId = MessageNS::tMessageId::MGS_STATUS_WIFI_AP_CONNECTED;
            break;
        default:
            // do nothing
            break;
    }

    if (wMessageId != MessageNS::tMessageId::NONE)
    {
		/* Create message */
    	MessageNS::Message wMessage;
    	wMessage.mSource = MessageNS::tAddress::WIFI_MANAGER;
        wMessage.mDestination = MessageNS::tAddress::TIME_MANAGER;

		/* Set selected message ID */
		wMessage.mId = wMessageId;

        /* Send message */
        mpTaskObjects->mpCommunicationManager->SendMessage(wMessage);
    }
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
    bool wNotifyTask = false;

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
			wNotifyTask = true;
			break;

		case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
//TODO there are to much disconnection events if not possible connect to STA with saved SSID isn't exist. FIX IT
			LOG(LOG_DEBUG, "WiFiManager::HandleWifiEvent() Station disconnected from AP");
			wNotifyTask = true;
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

	if (wNotifyTask)
	{
		/* Create message */
		MessageNS::Message wMessage;
		wMessage.mSource = MessageNS::tAddress::WIFI_MANAGER;
		wMessage.mDestination = MessageNS::tAddress::WIFI_MANAGER;

		wMessage.mId = MessageNS::tMessageId::MGS_EVENT_WIFI_EVENT_TRIGGERED;

		/* Serialize wifi event in message payload */
		if (SerializeNS::SerializeData(static_cast<uint8_t>(aEvent), wMessage.mPayload) == sizeof(uint8_t))
		{
			/* Set payload length */
			wMessage.mPayloadLength = sizeof(uint8_t);
			/* Send message */
			mpTaskObjects->mpCommunicationManager->SendMessage(wMessage);
		}
		else
		{
			//TODO handle serialization error
		}
	}
}