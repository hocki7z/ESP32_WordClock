/*
 * Language.h
 *
 * Centralized web UI translations.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace LanguagesNS
{
    enum class Language : uint8_t
    {
        English = 0,
        German = 1,
    };

    struct Translation
    {
        const char* mpEnglish;
        const char* mpGerman;
    };

    inline constexpr Translation mcTranslations[] = {
        {"WiFi scan ongoing, please wait...", "WLAN-Suche läuft, bitte warten..."},
        {"Overview", "Übersicht"},
        {"LED Display", "LED-Anzeige"},
        {"Settings", "Einstellungen"},
        {"Preview", "Vorschau"},
        {"Display", "Anzeige"},
        {"Clock mode", "Uhrmodus"},
        {"Show 'IT IS'", "'ES IST' anzeigen"},
        {"Show single minutes", "Einzelne Minuten anzeigen"},
        {"Colors", "Farben"},
        {"Day", "Tag"},
        {"time", "Zeit"},
        {"background", "Hintergrund"},
        {"Night", "Nacht"},
        {"Brightness", "Helligkeit"},
        {"Use day/night mode", "Tag-/Nachtmodus verwenden"},
        {"Day mode", "Tagesmodus"},
        {"start time", "Startzeit"},
        {"end time", "Endzeit"},
        {"General", "Allgemein"},
        {"Language", "Sprache"},
        {"Time", "Zeit"},
        {"NTP server", "NTP-Server"},
        {"Time zone", "Zeitzone"},
        {"WiFi Access Point Credentials", "WLAN-Zugangsdaten"},
        {"SSID", "WLAN-Name (SSID)"},
        {"Password", "Passwort"},
        {"Show/Hide Password", "Passwort anzeigen/ausblenden"},
        {"Save & Connect", "Speichern & verbinden"},
        {"Search for WiFi", "WLAN suchen"},
        {"Forget network", "Netzwerk vergessen"},
        {"System Information", "Systeminformationen"},
        {"Firmware Update", "Firmware-Update"},
        {"Software version", "Softwareversion"},
        {"Build date/time", "Build-Datum/-Uhrzeit"},
        {"Runtime", "Laufzeit"},
        {"Restart reason", "Neustartgrund"},
        {"ESP Flash usage", "ESP-Flash-Auslastung"},
        {"ESP Heap usage", "ESP-Heap-Auslastung"},
        {"ESP MaxAllocHeap", "ESP maximaler Heap-Block"},
        {"ESP MinFreeHeap", "ESP minimaler freier Heap"},
        {"Unknown", "Unbekannt"},
        {"Unavailable", "Nicht verfügbar"},
        {"Power-on", "Einschalten"},
        {"Software request", "Softwareanforderung"},
        {"Watchdog timeout", "Watchdog-Zeitüberschreitung"},
        {"Panic", "Panik"},
        {"Brownout detection", "Unterspannungserkennung"},
        {"Firmware update", "Firmware-Update"},
        {"Invalid reason", "Ungültiger Grund"},
        {"Australia Eastern Standard Time", "Australische Ostküstenzeit"},
        {"Australia Central Standard Time", "Australische Zentralzeit"},
        {"Afghanistan Time", "Afghanistan-Zeit"},
        {"Alaska Standard Time (DTS)", "Alaska-Zeit (Sommerzeit)"},
        {"Atlantic Standard Time", "Atlantische Standardzeit"},
        {"Central Africa Time", "Zentralafrikanische Zeit"},
        {"Central European Time (DTS)", "Mitteleuropäische Zeit (Sommerzeit)"},
        {"Central Standard Time (DTS)", "Nordamerikanische Zentralzeit (Sommerzeit)"},
        {"East Africa Time (DTS)", "Ostafrikanische Zeit"},
        {"Eastern European Time (DTS)", "Osteuropäische Zeit (Sommerzeit)"},
        {"Eastern Standard Time (DTS)", "Nordamerikanische Ostküstenzeit (Sommerzeit)"},
        {"Moscow Standard Time", "Moskauer Standardzeit"},
        {"Mountain Standard Time (DTS)", "Rocky-Mountain-Zeit (Sommerzeit)"},
        {"Pacific Standard Time (DTS)", "Pazifische Zeit (Sommerzeit)"},
        {"West Africa Time", "Westafrikanische Zeit"},
        {"Western European Time (DTS)", "Westeuropäische Zeit (Sommerzeit)"},
    };

    inline const char* GetText(const char* apEnglish, Language aLanguage)
    {
        if ((apEnglish == nullptr) || (aLanguage == Language::English))
        {
            return (apEnglish != nullptr) ? apEnglish : "";
        }

        for (const Translation& wTranslation : mcTranslations)
        {
            if (std::strcmp(apEnglish, wTranslation.mpEnglish) == 0)
            {
                return wTranslation.mpGerman;
            }
        }

        return apEnglish;
    }

    inline const char* GetUptimeFormat(Language aLanguage)
    {
        return (aLanguage == Language::German) ? "%uT %02uStd %02uMin" : "%ud %02uh %02um";
    }
}