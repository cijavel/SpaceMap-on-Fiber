#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include "Configuration.h"

// Runtime-Konfiguration, die per Webinterface geändert und im NVS
// (Non-Volatile Storage) des ESP32 dauerhaft gespeichert werden kann.
// Defaults kommen aus Configuration.h.
class AppConfig {
public:
    static AppConfig& getInstance() {
        static AppConfig instance;
        return instance;
    }

    // Lade gespeicherte Werte aus NVS (oder Defaults falls leer)
    void load();

    // Speichere alle Werte in NVS
    void save();

    // Setze alle Werte auf die Defaults aus Configuration.h zurück
    void resetToDefaults();

    // --- Getter ---
    unsigned long getIntervalWifiCheck()    const { return _intervalWifiCheck; }
    unsigned long getIntervalLEDs()         const { return _intervalLEDs; }
    unsigned long getIntervalApi()          const { return _intervalApi; }
    String        getSpaceApiUrl()          const { return _spaceApiUrl; }
    uint8_t       getLedBrightness()        const { return _ledBrightness; }
    uint8_t       getOnboardBrightness()    const { return _onboardBrightness; }
    uint16_t      getLedCount()             const { return _ledCount; }
    uint8_t       getLedDataPin()           const { return _ledDataPin; }
    uint16_t      getLedMaxPowerMa()        const { return _ledMaxPowerMa; }

    // --- Setter (ändern nur den RAM-Wert; save() für Persistenz aufrufen) ---
    void setIntervalWifiCheck(unsigned long v)  { _intervalWifiCheck = v; }
    void setIntervalLEDs(unsigned long v)       { _intervalLEDs = v; }
    void setIntervalApi(unsigned long v)        { _intervalApi = v; }
    void setSpaceApiUrl(const String& v)        { _spaceApiUrl = v; }
    void setLedBrightness(uint8_t v)            { _ledBrightness = v; }
    void setOnboardBrightness(uint8_t v)        { _onboardBrightness = v; }
    void setLedCount(uint16_t v)                { _ledCount = v; }
    void setLedDataPin(uint8_t v)               { _ledDataPin = v; }
    void setLedMaxPowerMa(uint16_t v)           { _ledMaxPowerMa = v; }

private:
    AppConfig() {}
    AppConfig(const AppConfig&) = delete;
    void operator=(const AppConfig&) = delete;

    Preferences _prefs;

    unsigned long _intervalWifiCheck  = interval_in_Seconds_WiFiCheck;
    unsigned long _intervalLEDs       = interval_in_Seconds_LEDs;
    unsigned long _intervalApi        = interval_in_Seconds_api;
    String        _spaceApiUrl        = webpage_SpaceAPI;
    uint8_t       _ledBrightness      = LED_BRIGHTNESS;
    uint8_t       _onboardBrightness  = ONBOARD_BRIGHTNESS;
    uint16_t      _ledCount           = LED_COUNT;
    uint8_t       _ledDataPin         = LED_DATA_PIN;
    uint16_t      _ledMaxPowerMa      = LED_MAX_POWER_MILLIAMPS;
};

#endif // APP_CONFIG_H