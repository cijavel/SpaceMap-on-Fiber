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
// SpaceMap persistence — uses a dedicated _smPrefs handle so it never
// conflicts with the settings _prefs handle.
// --------------------------------------------------------------------------
int AppConfig::loadSpaceMap(uint8_t* ledOut, String* nameOut, String* cityOut, bool* disabledOut, int maxEntries) {
    // Use readOnly=false — on ESP32, begin() with readOnly=true fails if the
    // namespace has never been written before (e.g. fresh flash or after clear()).
    if (!_smPrefs.begin("smdata", /*readOnly=*/false)) return 0;
    int count = (int)_smPrefs.getInt("smCount", 0);
    if (count <= 0 || count > maxEntries) { _smPrefs.end(); return 0; }
    for (int i = 0; i < count; i++) {
        ledOut[i]      = (uint8_t)_smPrefs.getInt(("smL" + String(i)).c_str(), 0);
        nameOut[i]     = _smPrefs.getString(("smN" + String(i)).c_str(), "");
        cityOut[i]     = _smPrefs.getString(("smC" + String(i)).c_str(), "");
        disabledOut[i] = _smPrefs.getBool(("smD" + String(i)).c_str(), false);
    }
    _smPrefs.end();
    return count;
}

void AppConfig::saveSpaceMap(const uint8_t* led, const String* name, const String* city, const bool* disabled, int count) {
    if (!_smPrefs.begin("smdata", /*readOnly=*/false)) return;
    _smPrefs.putInt("smCount", count);
    for (int i = 0; i < count; i++) {
        _smPrefs.putInt(("smL" + String(i)).c_str(), led[i]);
        _smPrefs.putString(("smN" + String(i)).c_str(), name[i]);
        _smPrefs.putString(("smC" + String(i)).c_str(), city[i]);
        _smPrefs.putBool(("smD" + String(i)).c_str(), disabled[i]);
    }
    _smPrefs.end();
}

void AppConfig::resetSpaceMap() {
    if (!_smPrefs.begin("smdata", /*readOnly=*/false)) return;
    _smPrefs.clear();
    _smPrefs.end();
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
