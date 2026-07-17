/*
 * WiFiManager.cpp
 *
 *  Created on: 13.09.2025
 *      Author: hocki
 */

/* Start Wifi connection with defined SSID and password */
//#define USE_CREDENTIALS

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
#include <WiFiClient.h>

#include <esp_wifi.h>

#include "Logger.h"
#include "Configuration.h"
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

    mpTimer = new ApplicationNS::TaskTimer(mPeriodicalTaskTimerId, 10000, true); // Period 10 seconds
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

        case MessageNS::tMessageId::CMD_WIFI_CONNECT:
        {
            /* Reset state to idle */
            mState = STATE_IDLE;
            /* Process state machine */
            ProcessState();
        }
            break;

        case MessageNS::tMessageId::CMD_WIFI_START_SCAN:
        {
            /* Start async scan (non-blocking) */
            WiFi.scanNetworks(true); // true = async
        }
            break;

        default:
            // do nothing
            break;
    }
}

void WiFiManager::ProcessState(const WiFiEvent_t aEvent)
{
    /* LOG */
    LOG(LOG_VERBOSE, "WiFiManager::ProcessState() Current state: %d, Event: %d", mState, aEvent);

    /* Process some events without changing current state */
    switch (aEvent)
    {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        case ARDUINO_EVENT_WIFI_AP_START:
            if (!mWifiScanRunOnce)
            {   
                /* LOG */
                LOG(LOG_DEBUG, "WiFiManager::HandleWifiEvent() Start async scan (non-blocking)");
                /* Start async scan (non-blocking) */
//                WiFi.scanNetworks(true); // true = async

                /* Set flag to indicate that WiFi scan has been run once */
//                mWifiScanRunOnce = true;
            }
            else
            {
                /* LOG */
                LOG(LOG_DEBUG, "WiFiManager::HandleWifiEvent() WiFi scan already run once, skipping");
            }
            break;

        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            /* Handle scan finished */
            HandleWiFiScanFinished();
            break;

        default:  
            // do nothing here
            break;
    }

    /* Process state machine */
    switch (mState)
    {
        case STATE_IDLE:
            /* Try connect to wifi router or start access point */
            if (ConnectWifi() == false)
            {
                ConnectAP();
            }

            /* Move to next state */
            mState = STATE_CONNECTING;
            break;

        case STATE_CONNECTING:
        case STATE_RECONNECTING:
            switch (aEvent)
            {
                case ARDUINO_EVENT_WIFI_STA_CONNECTED:
                    /* LOG */
                    LOG(LOG_DEBUG, "WiFiManager::ProcessState() Connected to wifi router after %d millis", (millis() - mConnectionStart));
                    /* Move to the next state*/
                    mState  = STATE_STA_CONNECTED;
                    /* Notify */
                    SendMessage(MessageNS::tMessageId::MSG_EVENT_WIFI_STA_CONNECTED);
                    break;

                case ARDUINO_EVENT_WIFI_AP_START:
                    /* LOG */
                    LOG(LOG_DEBUG, "WiFiManager::ProcessState() Access point started");

                    /* Start DNS server for captive portal */
                    if (mDnsServer.start(ConfigNS::mDnsPort, "*", WiFi.softAPIP()) == false)
                    {
                        LOG(LOG_ERROR, "WebSite::ProcessState() Start DNS server for captive portal failed");
                    }

                    /* Move to the next state*/
                    mState  = STATE_AP_STARTED;
                    /* Notify */
                    SendMessage(MessageNS::tMessageId::MSG_EVENT_WIFI_AP_STARTED);
                    break;
                
                default:
                    /* Check for connection timeout */
                    if (((millis() - mConnectionStart) >= mConnectionTimeout) &&
                        (WiFi.status() != WL_CONNECTED))
                    {
                        /* LOG */
                        LOG(LOG_ERROR, "WiFiManager::ProcessState() Failed to connect after %d millis", mConnectionTimeout);

                        /* Update connection start time */
                        mConnectionStart = millis();

                        /* Reconnect to wifi router */
                        WiFi.reconnect();

                        /* Move or stay in reconnecting state */
                        mState  = STATE_RECONNECTING;

                        /* Notify */
                        SendMessage(MessageNS::tMessageId::MSG_EVENT_WIFI_STA_DISCONNECTED);
                    }
                    break;
            };
            break;

        case STATE_STA_CONNECTED:
            switch (aEvent)
            {
                case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                    /* LOG */
                    LOG(LOG_DEBUG, "WiFiManager::ProcessState() Event got IP from connected wifi router %s",
                            WiFi.localIP().toString().c_str());

                    if (IsInternetAvailable())
                    {
                        LOG(LOG_DEBUG, "WiFiManager::ProcessState() We are online");
                        /* Notify */
                        SendMessage(MessageNS::tMessageId::MSG_EVENT_WIFI_INTERNET_AVAILABLE);
                    }
                    else
                    {
                        LOG(LOG_ERROR, "WiFiManager::ProcessState() Internet is NOT available");
                    }
                    break;

                case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                    /* LOG */
                    LOG(LOG_DEBUG, "WiFiManager::ProcessState() Event disconnected from wifi router");

                    /* Move to the next state */
                    mState  = STATE_RECONNECTING;
                    /* Notify */
                    SendMessage(MessageNS::tMessageId::MSG_EVENT_WIFI_STA_DISCONNECTED);
                    break;

                default:
                    break;
            };
            break;

        case STATE_AP_STARTED:
            switch (aEvent)
            {
                case ARDUINO_EVENT_WIFI_AP_STOP:
                    /* LOG */
                    LOG(LOG_DEBUG, "WiFiManager::ProcessState() Event access point stopped");

                    /* Move to the IDLE state only if the scan is not running */
                    if (WiFi.scanComplete() != WIFI_SCAN_RUNNING)
                    {
                        /* Move to the next state */
                        mState = STATE_IDLE;
                        /* Notify */
                        SendMessage(MessageNS::tMessageId::MSG_EVENT_WIFI_AP_STOPPED);
                    }
                    break;

                default:
                    break;
            };
            break;

        default:
            // do nothing
            break;
    }
}

void WiFiManager::HandleWiFiScanFinished(void)
{
    /* Save scan results */
    for (int wI = 0; wI < WiFi.scanComplete(); wI++)
    {
        ConfigNS::tSSIDEntry wEntry;

        // Put SSID, RSSI and encryption type
        strncpy(wEntry.mSsid, WiFi.SSID(wI).c_str(), sizeof(wEntry.mSsid) - 1);
        wEntry.mSsid[sizeof(wEntry.mSsid) - 1] = '\0';
        wEntry.mRssi = WiFi.RSSI(wI);
        wEntry.mEncrypted = WiFi.encryptionType(wI) != WIFI_AUTH_OPEN;

        // Search for duplicates in the list
        bool wDuplicateFound = false;
        for (const auto& existingEntry : ConfigNS::mSSSIDList)
        {
            if (strcmp(existingEntry.mSsid, wEntry.mSsid) == 0)
            {
                wDuplicateFound = true;
                break;
            }
        }

        if (!wDuplicateFound)
        {
            // Add entry to the list
            ConfigNS::mSSSIDList.push_back(wEntry);
        }
    }

    /* LOG */
    LOG(LOG_DEBUG, "WiFiManager::HandleWiFiScanFinished() WiFi scan done, found %d networks", ConfigNS::mSSSIDList.size());

#if (LOG_LEVEL == LOG_VERBOSE)  // don't compile this code each time
    /* Log all found network */
    for (const auto& entry : ConfigNS::mSSSIDList)
    {
        LOG(LOG_VERBOSE, "WiFiManager::HandleWiFiScanFinished()     %s, RSSI: %d dBm, Encryption: %s",
                entry.mSsid, entry.mRssi, entry.mEncrypted ? "Encrypted" : "Open");
    }
#endif /* (LOG_LEVEL == LOG_VERBOSE) */

    SendMessage(MessageNS::tMessageId::MSG_EVENT_WIFI_SCAN_DONE);
}

void WiFiManager::SendMessage(MessageNS::tMessageId wMessageId)
{
    /* Create message */
    MessageNS::Message wMessage;
    wMessage.mSource = MessageNS::tAddress::WIFI_MANAGER;

    /* Set selected message ID */
    wMessage.mId = wMessageId;

    /* Send message to the time manager */
    wMessage.mDestination = MessageNS::tAddress::TIME_MANAGER;
    Task::SendMessage(wMessage);

    /* Send message to the web manager */
    wMessage.mDestination = MessageNS::tAddress::WEB_MANAGER;
    Task::SendMessage(wMessage);
}

/**
 * @brief Check if internet is available by connecting to the Google's DNS server
 *
 * @return true if internet is available, false otherwise
 */
bool WiFiManager::IsInternetAvailable(void)
{
    bool wRetValue = false;

    if (WiFi.status() == WL_CONNECTED)
    {
        /* Create a client object */
        WiFiClient wWiFiClient;

        /* Set a short timeout (e.g. 500 ms) */
        wWiFiClient.setTimeout(500);

        /* Try to connect to Google's DNS server */
        wRetValue = wWiFiClient.connect("8.8.8.8", 53);

        /* Disconnect */
        wWiFiClient.stop();
    }

    return wRetValue;
}

/**
 * @brief Connect to a WiFi network using stored credentials or SDK configuration
 * 
 * @details This function attempts to connect to a WiFi network using either stored credentials (if USE_CREDENTIALS is defined)
 *          or the SDK configuration. It handles the necessary WiFi mode settings and connection initiation.
 * 
 * @return true if the connection attempt was initiated successfully, false otherwise
 */
bool WiFiManager::ConnectWifi(void)
{
    bool wRetValue = false;

    /* Disconnect from the network (close AP) */
    WiFi.softAPdisconnect(true);
    /* Disconnect from the network */
    WiFi.disconnect(true);

    /* Wait a moment */
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

    /* Wait a moment */
    delay(100);

    /* Set connection start time */
    mConnectionStart = millis();

#ifdef USE_CREDENTIALS
    /* LOG */
    LOG(LOG_DEBUG, "WiFiManager::ConnectWifi() Start WiFi Station mode, credentials SSID: %s", CRED_WIFI_SSID);
    /* Start Wifi connection */
    WiFi.begin(CRED_WIFI_SSID, CRED_WIFI_PASS);

    wRetValue = true;
#else

    /* Get SSID and password from settings */
    String wSsid  = Settings.GetValue<String>(ConfigNS::mKeyWifiSSID, "");
    String wPassw = Settings.GetValue<String>(ConfigNS::mKeyWifiPassword, "");

    /* Check if SSID is valid */
    if (wSsid.length() > 0)
    {
        /* LOG */
        LOG(LOG_DEBUG, "WiFiManager::ConnectWifi() Start WiFi Station mode, SSID: %s", wSsid.c_str());
        /* Start Wifi connection */
        WiFi.begin(wSsid.c_str(), wPassw.c_str());
        
        wRetValue = true;
    }
    else
    {
        LOG(LOG_ERROR, "WiFiManager::ConnectWifi() Failed to get WiFi SSID from settings");
    }
#endif /* ifdef USE_CREDENTIALS */

    return wRetValue;
}

/**
 * @brief Set the ESP32 as an access point
 */
void WiFiManager::ConnectAP(void)
{
    /* Disconnect from the network (close AP) */
    WiFi.softAPdisconnect(true);
    /* Disconnect from the network */
    WiFi.disconnect(true);
    /* Wait a moment */
    delay(100);

    /* Set WiFi soft-AP mode */
    LOG(LOG_DEBUG, "WifiMangerClass::ConnectAP() Start AP/STA mode");
    WiFi.mode(WIFI_AP_STA);
    /* Wait a moment */
    delay(100);

    /* Start WiFi AP connection */
    if (WiFi.softAP(ConfigNS::mWiFiApSSID, ConfigNS::mWiFiApPASS) == true)
    {
        LOG(LOG_VERBOSE, "WifiMangerClass::ConnectAP() Access Point %s [%s] started",
                ConfigNS::mWiFiApSSID, WiFi.softAPIP().toString().c_str());
    }
    else
    {
        LOG(LOG_ERROR, "WifiMangerClass::ConnectAP() Setup access point failed");
    }
}

void WiFiManager::HandleWifiEvent(WiFiEvent_t aEvent)
{
    bool wNotifyTask = false;

    LOG(LOG_VERBOSE, "WiFiManager::HandleWifiEvent() Event: %d", aEvent);

    switch (aEvent)
    {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
//TODO there are to much disconnection events if not possible connect to STA with saved SSID isn't exist. FIX IT
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        case ARDUINO_EVENT_WIFI_AP_START:
        case ARDUINO_EVENT_WIFI_AP_STOP:
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            /* Notify WifiManager task */
            wNotifyTask = true;
            break;

        case ARDUINO_EVENT_WIFI_STA_START:
        case ARDUINO_EVENT_WIFI_STA_STOP:
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
        case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
        default:
            // do nothing
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