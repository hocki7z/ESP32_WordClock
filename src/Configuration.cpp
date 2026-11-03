/*
 * Configuration.cpp
 *
 *  Created on: 10.07.2026
 *      Author: hocki
 */
#include "Configuration.h"


/** @brief Shared list instance of discovered WiFi SSIDs */
SSIDList mSSIDList;

/** @brief Shared display-scene snapshot */
DisplaySceneSnapshot mDisplaySceneSnapshot;

/** @brief Retrieve the product information from the flash partition. */
bool ConfigNS::GetProductInfo(ProductInfoNS::Record& arRecord)
{
	return ProductInfoNS::ReadFlashPartition(arRecord);
}