#include "AppConfig.h"

static const char* NVS_NS = "spacemap";  // NVS Namespace (max 15 Zeichen)

void AppConfig::load() {
    if (!_prefs.begin(NVS_NS, /*readOnly=*/false)) {
#ifdef DEBUG
        Serial.println("CFG: NVS open failed in load()");
#endif
        return; // Defaults aus Konstruktor bleiben erhalten
    }

    _intervalWifiCheck  = _prefs.getULong("wifiInterval",  interval_in_Seconds_WiFiCheck);
    _intervalLEDs       = _prefs.getULong("ledInterval",   interval_in_Seconds_LEDs);
    _intervalApi        = _prefs.getULong("apiInterval",   interval_in_Seconds_api);
    _spaceApiUrl        = _prefs.getString("apiUrl",       webpage_SpaceAPI);
    _ledBrightness      = _prefs.getUChar("ledBright",     LED_BRIGHTNESS);
    _onboardBrightness  = _prefs.getUChar("obBright",      ONBOARD_BRIGHTNESS);
    _ledMaxPowerMa      = _prefs.getUShort("ledMaxPwr",    LED_MAX_POWER_MILLIAMPS);

    _prefs.end();
}

void AppConfig::save() {
    if (!_prefs.begin(NVS_NS, /*readOnly=*/false)) {
#ifdef DEBUG
        Serial.println("CFG: NVS open failed in save()");
#endif
        return;
    }

    _prefs.putULong("wifiInterval", _intervalWifiCheck);
    _prefs.putULong("ledInterval",  _intervalLEDs);
    _prefs.putULong("apiInterval",  _intervalApi);
    _prefs.putString("apiUrl",      _spaceApiUrl);
    _prefs.putUChar("ledBright",    _ledBrightness);
    _prefs.putUChar("obBright",     _onboardBrightness);
    _prefs.putUShort("ledMaxPwr",   _ledMaxPowerMa);

    _prefs.end();
}

void AppConfig::resetToDefaults() {
    _intervalWifiCheck  = interval_in_Seconds_WiFiCheck;
    _intervalLEDs       = interval_in_Seconds_LEDs;
    _intervalApi        = interval_in_Seconds_api;
    _spaceApiUrl        = webpage_SpaceAPI;
    _ledBrightness      = LED_BRIGHTNESS;
    _onboardBrightness  = ONBOARD_BRIGHTNESS;
    _ledMaxPowerMa      = LED_MAX_POWER_MILLIAMPS;
    save();  // direkt in NVS schreiben
}