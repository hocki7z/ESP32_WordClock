/*
 * Configuration.cpp
 *
 *  Created on: 10.07.2026
 *      Author: hocki
 */
#include "Configuration.h"

bool ConfigNS::GetProductInfo(ProductInfoNS::Record& arRecord)
{
	return ProductInfoNS::ReadFlashPartition(arRecord);
}

/** @brief Shared list instance of discovered WiFi SSIDs */
SSIDList mSSIDList;

/** @brief Shared display-scene snapshot */
DisplaySceneSnapshot mDisplaySceneSnapshot;
