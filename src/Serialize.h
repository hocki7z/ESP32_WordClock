/*
 * Serialize.h
 *
 *  Created on: 12.10.2025
 *      Author: hocki
 */
#pragma once

#include <Arduino.h>

namespace SerializeNS
{
    /**
     * @brief Store 1 byte data into byte array
     *
     * @param aData         : Data to store
     * @param apDestination : Byte array in the data should be stored
     * @param aOffset       : Index within the byte array at which the data should be stored
     *
     * @return aOffset + 1, can be used as offset for next call
     */
    inline uint32_t SerializeData(const uint8_t aData, uint8_t* const apDestination, const uint32_t aOffset = 0)
    {
        uint32_t wOffset = aOffset;

        /* Copy data as little endian */
        apDestination[wOffset++] = (aData >> 0) & 0xFF;

        return wOffset;
    }

    /**
     * @brief Store 2-byte halfword into byte array
     *
     * @param aData         : Data to store
     * @param apDestination : Byte array in the data should be stored
     * @param aOffset       : Index within the byte array at which the data should be stored
     *
     * @return parOffset + 2, can be used as offset for next call
     */
    inline uint32_t SerializeData(const uint16_t aData, uint8_t* const apDestination, uint32_t aOffset = 0)
    {
        /* Copy data as little endian */
        apDestination[aOffset++] = (aData >> 0) & 0xFF;
        apDestination[aOffset++] = (aData >> 8) & 0xFF;

        return aOffset;
    }

    /**
     * @brief Store 4-byte word into byte array
     *
     * @param aData         : Data to store
     * @param apDestination : Byte array in the data should be stored
     * @param aOffset       : Index within the byte array at which the data should be stored
     *
     * @return aOffset + 4, can be used as offset for next call
     */
    inline uint32_t SerializeData(const uint32_t aData, uint8_t* const apDestination, uint32_t aOffset = 0)
    {
        /* Copy data as little endian */
        apDestination[aOffset++] = (aData >>  0) & 0xFF;
        apDestination[aOffset++] = (aData >>  8) & 0xFF;
        apDestination[aOffset++] = (aData >> 16) & 0xFF;
        apDestination[aOffset++] = (aData >> 24) & 0xFF;

        return aOffset;
    }

    /**
     * @brief Store 4-bytes float into byte array
     *
     * @param aData         : Data to store
     * @param apDestination : Byte array in the data should be stored
     * @param aOffset       : Index within the byte array at which the data should be stored
     *
     * @return aOffset + 4, can be used as offset for next call
     */
    inline uint32_t SerializeData(const float aData, uint8_t* const apDestination, uint32_t aOffset = 0)
    {
        union
        {
            float    wFloating;
            uint32_t wRawData;
        };
        wFloating = aData;

        /* Copy data as little endian */
        apDestination[aOffset++] = (wRawData >>  0) & 0xFF;
        apDestination[aOffset++] = (wRawData >>  8) & 0xFF;
        apDestination[aOffset++] = (wRawData >> 16) & 0xFF;
        apDestination[aOffset++] = (wRawData >> 24) & 0xFF;

        return aOffset;
    }

    /**
     * @brief Retrieve 1-byte data from a serialized byte array
     *
     * @param[in]  apByteArray : Byte array from which the data should be read
     * @param[out] apData      : Pointer to the data retrieved from the byte array
     * @param[in]  aOffset     : Index within the byte array at which the data is located
     *
     * @return aOffset + 2, can be used as offset for next call
     */
    inline uint32_t DeserializeData(const uint8_t* const apByteArray, uint8_t* const apData, uint32_t aOffset = 0)
    {
        /* Retrieve data from byte array as little endian */
        *apData = (apByteArray[aOffset++] <<  0);

        return aOffset;
    }

    /**
     * @brief Retrieve 2-byte halfword from a serialized byte array
     *
     * @param[in]  apByteArray : Byte array from which the data should be read
     * @param[out] apData      : Pointer to the data retrieved from the byte array
     * @param[in]  aOffset     : Index within the byte array at which the data is located
     *
     * @return aOffset + 2, can be used as offset for next call
     */
    inline uint32_t DeserializeData(const uint8_t* const apByteArray, uint16_t* const apData, uint32_t aOffset = 0)
    {
        /* Retrieve data from byte array as little endian */
        *apData = static_cast<uint16_t>(
            (static_cast<uint32_t>(apByteArray[aOffset    ]) <<  0) |
            (static_cast<uint32_t>(apByteArray[aOffset + 1]) <<  8));

        return aOffset + 2;
    }

    /**
     * @brief Retrieve 4-byte word from a serialized byte array
     *
     * @param[in]  apByteArray : Byte array from which the data should be read
     * @param[out] apData      : Pointer to the data retrieved from the byte array
     * @param[in]  aOffset     : Index within the byte array at which the data is located
     *
     * @return aOffset + 4, can be used as offset for next call
     */
    inline uint32_t DeserializeData(const uint8_t* const apByteArray, uint32_t* const apData, uint32_t aOffset = 0)
    {
        /* Retrieve data from byte array as little endian */
        *apData = (static_cast<uint32_t>(apByteArray[aOffset    ]) <<  0) |
                  (static_cast<uint32_t>(apByteArray[aOffset + 1]) <<  8) |
                  (static_cast<uint32_t>(apByteArray[aOffset + 2]) << 16) |
                  (static_cast<uint32_t>(apByteArray[aOffset + 3]) << 24);

        return aOffset + 4;
    }

    /**
     * @brief Retrieve 4-byte float from a serialized byte array
     *
     * @param[in]  apByteArray : Byte array from which the data should be read
     * @param[out] apData      : Pointer to the data retrieved from the byte array
     * @param[in]  aOffset     : Index within the byte array at which the data is located
     *
     * @return aOffset + 4, can be used as offset for next call
     */
    inline uint32_t DeserializeData(const uint8_t* const apByteArray, float* const apData, uint32_t aOffset = 0)
    {
        /* Use union in order to split float into 4 bytes */
        union
        {
            float wFloating;
            uint32_t wRawData;
        };

        /* Retrieve data from byte array as little endian */
        wRawData = (static_cast<uint32_t>(apByteArray[aOffset    ]) <<  0) |
               (static_cast<uint32_t>(apByteArray[aOffset + 1]) <<  8) |
               (static_cast<uint32_t>(apByteArray[aOffset + 2]) << 16) |
               (static_cast<uint32_t>(apByteArray[aOffset + 3]) << 24);

        /* Set float value */
        *apData = wFloating;

        return aOffset + 4;
    }

}; /* end of namespace SerializeNS */
