/*
 * Communication.cpp
 *
 *  Created on: 12.10.2025
 *      Author: hocki
 */

#include "Communication.h"
#include <cassert>

namespace CommunicationNS
{
/**
 * @brief Constructor
 */
CommunicationManager::CommunicationManager()
{
    // do nothing
}

/**
 * @brief Destructor
 */
CommunicationManager::~CommunicationManager()
{
    // do nothing
}

void CommunicationManager::RegisterCallback(MessageNS::tAddress aAddress, NotificationCallback * apCallback)
{
    /* Check input arguments */
    if((aAddress < MessageNS::tAddress::NB_OF_ADDRESSES) &&
       (apCallback != nullptr))
    {
        /* Store callback */
        mpRegisteredCallbacks[aAddress] = apCallback;
    }
}

void CommunicationManager::SendMessage(const MessageNS::Message & apMessage) const
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

}   /* end of namespace CommunicationNS */