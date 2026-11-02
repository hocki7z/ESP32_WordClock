/*
 * Configuration.cpp
 *
 *  Created on: 10.07.2026
 *      Author: hocki
 */
#include "Configuration.h"

const ProductInfoNS::Record& ConfigNS::GetProductInfo()
{
	return ProductInfoNS::GetRecord();
}

/** @brief Shared list instance of discovered WiFi SSIDs */
SSIDList mSSIDList;

/** @brief Shared display-scene snapshot */
DisplaySceneSnapshot mDisplaySceneSnapshot;
