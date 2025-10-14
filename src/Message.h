/*
 * Meessage.h
 *
 *  Created on: 12.10.2025
 *      Author: hocki
 */

#ifndef SRC_MESSAGE_H_
#define SRC_MESSAGE_H_

#include <Arduino.h>

namespace MessageNS
{
    /**
     * @brief Address definition for source and destination modules
     */
    enum tAddress : uint8_t
    {
        APPLICATION_MANAGER = 0x00,

        DISPLAY_MANAGER,
        TIME_MANAGER,
        WIFI_MANAGER,
        WEB_MANAGER,

        /** @brief Total number of address (do not use as actual address) */
        NB_OF_ADDRESSES
    };

    /**
     * @brief Message ID
     */
    enum tMessageId : uint8_t
    {
        NONE = 0x00,

        /** @brief Number of IDs available (do not use as actual command) */
        NB_OF_MESSAGE_IDS
    };

    /**
     * @brief Definition of the structure of an message to communicate with a task.
     */
    template<uint8_t PayloadLen>
    struct tMessage
    {
        /** @brief Source address */
        tAddress mSource;
        /** @brief Destination address */
        tAddress mDestination;
        /** @brief Message ID */
        tMessageId mId;

        /** @brief Length of the payload */
        uint8_t  mPayloadLength;
        /** @brief Payload of the message */
        uint8_t  mPayload[PayloadLen];
    };

    /**@brief Length of internal message payload */
    constexpr uint8_t mMessagePayloadLen = 4;
    /** @brief Definition of internal message */
    using Message = MessageNS::tMessage<mMessagePayloadLen>;

    /**
     * @brief Convert an address to a string
     * @param aAddress Address to convert
     * @return Pointer to a string representing the address
     */
    inline const char* AddressToString(tAddress aAddress)
    {
        switch(aAddress)
        {
            case APPLICATION_MANAGER : return "APP_MGR";
            case DISPLAY_MANAGER     : return "DISP_MGR";
            case TIME_MANAGER        : return "TIME_MGR";
            case WIFI_MANAGER        : return "WIFI_MGR";
            case WEB_MANAGER         : return "WEB_MGR";
            default                  : return "UNKN_ADDR";
        }
    }

}; /* end of namespace MessageNS */

#endif /* SRC_MESSAGE_H_ */
