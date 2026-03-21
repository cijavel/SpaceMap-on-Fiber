#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include "Configuration.h"

// Runtime configuration that can be changed via the web interface and is
// persisted in the ESP32 Non-Volatile Storage (NVS).
// Compile-time defaults are defined in Configuration.h.
class AppConfig {
public:
    static AppConfig& getInstance() {
        static AppConfig instance;
        return instance;
    }

    // Load stored values from NVS; falls back to compile-time defaults if empty.
    void load();

    // Persist all current values to NVS.
    void save();

    // Reset all values to the compile-time defaults from Configuration.h and save.
    void resetToDefaults();

    // --- Getters ---
    unsigned long getIntervalWifiCheck()  const { return _intervalWifiCheck; }
    unsigned long getIntervalLEDs()       const { return _intervalLEDs; }
    unsigned long getIntervalApi()        const { return _intervalApi; }
    String        getSpaceApiUrl()        const { return _spaceApiUrl; }
    uint8_t       getLedBrightness()      const { return _ledBrightness; }
    uint8_t       getOnboardBrightness()  const { return _onboardBrightness; }
    uint16_t      getLedMaxPowerMa()      const { return _ledMaxPowerMa; }

    // --- Setters (update RAM only; call save() to persist) ---
    void setIntervalWifiCheck(unsigned long value) { _intervalWifiCheck = value; }
    void setIntervalLEDs(unsigned long value)      { _intervalLEDs = value; }
    void setIntervalApi(unsigned long value)       { _intervalApi = value; }
    void setSpaceApiUrl(const String& value)       { _spaceApiUrl = value; }
    void setLedBrightness(uint8_t value)           { _ledBrightness = value; }
    void setOnboardBrightness(uint8_t value)       { _onboardBrightness = value; }
    void setLedMaxPowerMa(uint16_t value)          { _ledMaxPowerMa = value; }

private:
    AppConfig() {}
    AppConfig(const AppConfig&) = delete;
    void operator=(const AppConfig&) = delete;

    Preferences _prefs;

    unsigned long _intervalWifiCheck = interval_in_Seconds_WiFiCheck;
    unsigned long _intervalLEDs      = interval_in_Seconds_LEDs;
    unsigned long _intervalApi       = interval_in_Seconds_api;
    String        _spaceApiUrl       = webpage_SpaceAPI;
    uint8_t       _ledBrightness     = LED_BRIGHTNESS;
    uint8_t       _onboardBrightness = ONBOARD_BRIGHTNESS;
    uint16_t      _ledMaxPowerMa     = LED_MAX_POWER_MILLIAMPS;
};

#endif // APP_CONFIG_H
