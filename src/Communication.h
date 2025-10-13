/*
 * Communication.h
 *
 *  Created on: 12.10.2025
 *      Author: hocki
 */
#ifndef SRC_COMMUNICATION_H_
#define SRC_COMMUNICATION_H_

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
    CommunicationManager() {}
    virtual ~CommunicationManager() {}

    virtual void RegisterCallback(MessageNS::tAddress aAddress, NotificationCallback * apCallback)
    {
        /* Check input arguments */
        if((aAddress < MessageNS::tAddress::NB_OF_ADDRESSES) &&
           (apCallback != nullptr))
        {
            /* Store callback */
            mpRegisteredCallbacks[aAddress] = apCallback;
        }
    }

    void SendMessage(const MessageNS::Message & apMessage) const
    {
        /* Check addresses */
        assert(apMessage.mSource      < MessageNS::tAddress::NB_OF_ADDRESSES);
        assert(apMessage.mDestination < MessageNS::tAddress::NB_OF_ADDRESSES);

        /* Check if a callback is exist */
        if(mpRegisteredCallbacks[apMessage.mDestination])
        {
            /* Call registered callback */
            mpRegisteredCallbacks[apMessage.mDestination]->NotifyMessage(apMessage);
        }
    }

private:
    /**
     * Include all registered callbacks. The position in the array represent the
     * address of the module.
     */
    NotificationCallback* mpRegisteredCallbacks[MessageNS::tAddress::NB_OF_ADDRESSES];
};

}; /* end of namespace CommunicationNS */

#endif /* SRC_COMMUNICATION_H_ */