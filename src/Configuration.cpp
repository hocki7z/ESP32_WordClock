/*
 * Configuration.cpp
 *
 *  Created on: 10.07.2026
 *      Author: hocki
 */
#include "Configuration.h"
#include <MutexCPP.h>

namespace ConfigNS
{
    namespace
    {
        Mutex mDisplayScenePreviewMutex("DisplayScenePreview");
    }

    /** @brief List of WiFi scan results - single shared instance */
    std::vector<tSSIDEntry> mSSSIDList;

    static DisplayScene mDisplayScenePreview;

    void StoreDisplayScenePreview(const DisplayScene& arScene)
    {
        Lock lock(mDisplayScenePreviewMutex);
        mDisplayScenePreview.CopyFrom(arScene);
    }

    void CopyDisplayScenePreview(DisplayScene& arScene)
    {
        Lock lock(mDisplayScenePreviewMutex);
        arScene.CopyFrom(mDisplayScenePreview);
    }
}
