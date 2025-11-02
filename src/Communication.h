/*
 * Communication.h
 *
 *  Created on: 12.10.2025
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>

#include "Message.h"


namespace CommunicationNS
{

class NotificationCallback
{
public:
    virtual void NotifyMessage(const MessageNS::Message & arMessage) = 0;
};

class CommunicationManager
{
public:
    CommunicationManager();
    virtual ~CommunicationManager();

    virtual void RegisterCallback(MessageNS::tAddress aAddress, NotificationCallback * apCallback);

    void SendMessage(const MessageNS::Message & apMessage) const;

private:
    /**
     * Include all registered callbacks. The position in the array represent the
     * address of the module.
     */
    NotificationCallback* mpRegisteredCallbacks[MessageNS::tAddress::NB_OF_ADDRESSES];
};

}; /* end of namespace CommunicationNS */
