#include "AppConfig.h"
#include "DataSpaceList.h"

// NVS namespace – must be 15 characters or fewer.
static const char* NVS_NAMESPACE = "spacemap";

void AppConfig::load() {
    if (!_prefs.begin(NVS_NAMESPACE, /*readOnly=*/false)) {
#ifdef DEBUG
        Serial.println("CFG: NVS open failed in load()");
#endif
        return; // Keep constructor defaults.
    }

    _intervalWifiCheck = _prefs.getULong("wifiInterval",  interval_in_Seconds_WiFiCheck);
    _intervalLEDs      = _prefs.getULong("ledInterval",   interval_in_Seconds_LEDs);
    _intervalApi       = _prefs.getULong("apiInterval",   interval_in_Seconds_api);
    _spaceApiUrl       = _prefs.getString("apiUrl",       webpage_SpaceAPI);
    _ledBrightness     = _prefs.getUChar("ledBright",     LED_BRIGHTNESS);
    _onboardBrightness = _prefs.getUChar("obBright",      ONBOARD_BRIGHTNESS);
    _ledMaxPowerMa     = _prefs.getUShort("ledMaxPwr",    LED_MAX_POWER_MILLIAMPS);

    _prefs.end();
}

void AppConfig::save() {
    if (!_prefs.begin(NVS_NAMESPACE, /*readOnly=*/false)) {
#ifdef DEBUG
        Serial.println("CFG: NVS open failed in save()");
#endif
        return;
    }

    _prefs.putULong("wifiInterval",  _intervalWifiCheck);
    _prefs.putULong("ledInterval",   _intervalLEDs);
    _prefs.putULong("apiInterval",   _intervalApi);
    _prefs.putString("apiUrl",       _spaceApiUrl);
    _prefs.putUChar("ledBright",     _ledBrightness);
    _prefs.putUChar("obBright",      _onboardBrightness);
    _prefs.putUShort("ledMaxPwr",    _ledMaxPowerMa);

    _prefs.end();
}

// --------------------------------------------------------------------------
// SpaceMap persistence
// --------------------------------------------------------------------------
int AppConfig::loadSpaceMap(uint8_t* ledOut, String* nameOut, String* cityOut, int maxEntries) {
    if (!_prefs.begin(NVS_NAMESPACE, /*readOnly=*/true)) return 0;
    int count = (int)_prefs.getInt("smCount", 0);
    if (count <= 0 || count > maxEntries) { _prefs.end(); return 0; }
    for (int i = 0; i < count; i++) {
        ledOut[i]  = (uint8_t)_prefs.getInt(("smL" + String(i)).c_str(), 0);
        nameOut[i] = _prefs.getString(("smN" + String(i)).c_str(), "");
        cityOut[i] = _prefs.getString(("smC" + String(i)).c_str(), "");
    }
    _prefs.end();
    return count;
}

void AppConfig::saveSpaceMap(const uint8_t* led, const String* name, const String* city, int count) {
    if (!_prefs.begin(NVS_NAMESPACE, /*readOnly=*/false)) return;
    _prefs.putInt("smCount", count);
    for (int i = 0; i < count; i++) {
        _prefs.putInt(("smL" + String(i)).c_str(), led[i]);
        _prefs.putString(("smN" + String(i)).c_str(), name[i]);
        _prefs.putString(("smC" + String(i)).c_str(), city[i]);
    }
    _prefs.end();
}

void AppConfig::resetSpaceMap() {
    if (!_prefs.begin(NVS_NAMESPACE, /*readOnly=*/false)) return;
    // Read the stored count first so we can clean up all individual entry keys.
    // Use SPACEMAP_MAX_ENTRIES as upper bound in case smCount itself is corrupt.
    int count = (int)_prefs.getInt("smCount", 0);
    if (count <= 0 || count > SPACEMAP_MAX_ENTRIES) count = SPACEMAP_MAX_ENTRIES;
    for (int i = 0; i < count; i++) {
        _prefs.remove(("smL" + String(i)).c_str());
        _prefs.remove(("smN" + String(i)).c_str());
        _prefs.remove(("smC" + String(i)).c_str());
    }
    _prefs.remove("smCount");
    _prefs.end();
}

void AppConfig::resetToDefaults() {
    _intervalWifiCheck = interval_in_Seconds_WiFiCheck;
    _intervalLEDs      = interval_in_Seconds_LEDs;
    _intervalApi       = interval_in_Seconds_api;
    _spaceApiUrl       = webpage_SpaceAPI;
    _ledBrightness     = LED_BRIGHTNESS;
    _onboardBrightness = ONBOARD_BRIGHTNESS;
    _ledMaxPowerMa     = LED_MAX_POWER_MILLIAMPS;
    save(); // Persist immediately after resetting.
}
