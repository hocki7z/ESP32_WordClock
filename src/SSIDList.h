/*
 * SSIDList.h
 *
 *  Created on: 25.10.2026
 *      Author: hocki
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include <MutexCPP.h>


/** @brief Maximum SSID name size, including null terminator */
#define SSID_NAME_SIZE      33                  // Wi‑Fi SSIDs are limited to 32 bytes + null terminator

/** @brief Maximum SSID password size, including null terminator */
#define SSID_PASSWORD_SIZE  65                  // Wi‑Fi passwords are limited to 64 bytes + null terminator

/** @brief Single WiFi entry */
struct tSSIDEntry
{
    char     mSsid[SSID_NAME_SIZE];             // Max SSID length is 32 + null terminator
    char     mPassword[SSID_PASSWORD_SIZE];     // Max WPA2 password length is 64 + null terminator
    int32_t  mRssi;
    bool     mEncrypted;

    /** @brief Compare two SSID entries by SSID
     *
     * @param arOther The other SSID entry to compare
     * @return true if the SSIDs are equal, false otherwise
     */
    bool operator==(const tSSIDEntry& arOther) const
    {
        return (std::strcmp(mSsid, arOther.mSsid) == 0);
    }

    /** @brief Check if the SSID entry is valid
     *
     * @return true if the SSID is non-empty, false otherwise
     */
    bool IsValid(void) const
    {
        return (std::strlen(mSsid) != 0);
    }

    /** @brief Clear the SSID entry */
    void Clear(void)
    {
        std::memset(mSsid, 0, sizeof(mSsid));
        std::memset(mPassword, 0, sizeof(mPassword));
        mRssi = 0;
        mEncrypted = false;
    }

    /** @brief Set the SSID entry
     *
     * @param apSsid The SSID string
     * @param apPassword The password string
     * @param aEncrypted Whether the network is encrypted
     * @param aRssi The RSSI value
     */
    void Set(const char* apSsid, const char* apPassword, bool aEncrypted = false, int32_t aRssi = 0)
    {
        if (apSsid != nullptr)
        {
            std::strncpy(mSsid, apSsid, sizeof(mSsid) - 1);
            mSsid[sizeof(mSsid) - 1] = '\0';
        }
        else
        {
            std::memset(mSsid, 0, sizeof(mSsid));
        }

        if (apPassword != nullptr)
        {
            std::strncpy(mPassword, apPassword, sizeof(mPassword) - 1);
            mPassword[sizeof(mPassword) - 1] = '\0';
        }
        else
        {
            std::memset(mPassword, 0, sizeof(mPassword));
        }

        mEncrypted = aEncrypted;
        mRssi = aRssi;
    }

    /** @brief Set the SSID entry from another entry
     *
     * @param arOther The other SSID entry to copy from
     */
    void Set(const tSSIDEntry& arOther)
    {
        Set(arOther.mSsid, arOther.mPassword, arOther.mEncrypted, arOther.mRssi);
    }
};

/** @brief Thread-safe owner of the discovered WiFi SSID entries */
class SSIDList
{
public:
    SSIDList() : mMutex("SSIDList")
    {
    }

    /** @brief Clear all SSID entries */
    void Clear(void)
    {
        FreeRTOScpp::Lock wLock(mMutex);
        mEntries.clear();
    }

    /** @brief Add an entry if it is not already in the list
     *
     * @param arEntry The SSID entry to add
     * @return true if the entry was added, false if it was a duplicate
     */
    bool AddIfUnique(const tSSIDEntry& arEntry)
    {
        FreeRTOScpp::Lock wLock(mMutex);
        return AddIfUniqueUnlocked(arEntry);
    }

    /** @brief Get the number of SSID entries */
    size_t Size(void) const
    {
        FreeRTOScpp::Lock wLock(mMutex);
        return mEntries.size();
    }

    /** @brief Copy an entry by index; false means the index is out of range
     *
     * @param aIndex The index of the entry to copy
     * @param arEntry The entry to copy into
     * @return true if the entry was copied, false if the index is out of range
     */
    bool Get(size_t aIndex, tSSIDEntry& arEntry) const
    {
        bool wRetValue = false;

        FreeRTOScpp::Lock wLock(mMutex);

        if (aIndex < mEntries.size())
        {
            arEntry.Set(mEntries[aIndex]);
            wRetValue = true;
        }

        return wRetValue;
    }

    /** @brief Exclusive RAII access for atomic multi-entry updates */
    class WriteAccess
    {
    public:

        /** @brief Clear all entries (caller must hold the lock) */
        void Clear()
        {
            mrList.mEntries.clear();
        }

        /** @brief Add an entry if it is not already in the list (caller must hold the lock)
         *
         * @param arEntry The SSID entry to add
         * @return true if the entry was added, false if it was a duplicate
         */
        bool AddIfUnique(const tSSIDEntry& arEntry)
        {
            return mrList.AddIfUniqueUnlocked(arEntry);
        }

    private:
        friend class SSIDList;

        explicit WriteAccess(SSIDList& arList)
            : mrList(arList),
              mLock(arList.mMutex)
        {
        }

        SSIDList& mrList;
        FreeRTOScpp::Lock mLock;
    };

    WriteAccess BeginUpdate()
    {
        return WriteAccess(*this);
    }

private:
    /** @brief Mutex protecting the entries */
    mutable FreeRTOScpp::Mutex mMutex;

    /** @brief List of SSID entries */
    std::vector<tSSIDEntry> mEntries;

    /** @brief Add an entry if it is not already in the list (caller must hold the lock)
     *
     * @param arEntry The SSID entry to add
     * @return true if the entry was added, false if it was a duplicate
     */
    bool AddIfUniqueUnlocked(const tSSIDEntry& arEntry)
    {
        bool wDuplicateFound = false;

        /* Search for duplicates in the list */
        for (const auto& wExistingEntry : mEntries)
        {
            if (arEntry == wExistingEntry)
            {
                wDuplicateFound = true;
                break;
            }
        }

        /* Add entry to the list if not a duplicate */
        if (!wDuplicateFound)
        {
            mEntries.push_back(arEntry);
        }

        return !wDuplicateFound;
    }
};

/** @brief Shared list instance; access entries only through synchronized methods */
extern SSIDList mSSIDList;
