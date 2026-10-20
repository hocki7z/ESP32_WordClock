/*
 * Configuration.cpp
 *
 *  Created on: 10.07.2026
 *      Author: hocki
 */
#include "Configuration.h"
#include <freertos/FreeRTOS.h>

namespace ConfigNS
{
    namespace
    {
        portMUX_TYPE mDisplayScenePreviewMux = portMUX_INITIALIZER_UNLOCKED;
    }

    /** @brief List of WiFi scan results - single shared instance */
    std::vector<tSSIDEntry> mSSSIDList;

    static DisplayScene mDisplayScenePreview;

    void StoreDisplayScenePreview(const DisplayScene& arScene)
    {
        portENTER_CRITICAL(&mDisplayScenePreviewMux);
        mDisplayScenePreview.CopyFrom(arScene);
        portEXIT_CRITICAL(&mDisplayScenePreviewMux);
    }

    void CopyDisplayScenePreview(DisplayScene& arScene)
    {
        portENTER_CRITICAL(&mDisplayScenePreviewMux);
        arScene.CopyFrom(mDisplayScenePreview);
        portEXIT_CRITICAL(&mDisplayScenePreviewMux);
    }
}
