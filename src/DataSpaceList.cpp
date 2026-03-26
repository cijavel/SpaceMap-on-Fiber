#include "DataSpaceList.h"
#include "AppConfig.h"
#include <memory>

// --------------------------------------------------------------------------
// Built-in default list of hackerspaces.
// Each entry maps a LED strip index to a space name and city.
// This list is used as the factory default when no custom mapping is stored.
// --------------------------------------------------------------------------
static const SpaceSearchList defaultSearchList[] = {
    { 0, "OpenLab Augsburg e.V."             , "Augsburg"},
    { 1, "IT-Syndikat"                       , "Innsbruck"},
    { 2, "muCCC"                             , "Munich"},
    { 3, "realraum"                          , "Graz"},
    { 4, "Binary Kitchen"                    , "Regensburg"},
    { 5, "c-base"                            , "Berlin"},
    { 6, "Chaosdorf"                         , "Dusseldorf"},
    { 7, "CCC Frankfurt"                     , "Frankfurt"},
    { 8, "backspace"                         , "Bamberg"},
    { 9, "CCCHH"                             , "Hamburg" },
    {10, "vspace.one"                        , "VS-Villingen"},
    {11, "Nerdberg"                          , "Fuerth"},
    {12, "dezentrale"                        , "Leipzig"},
};
static const int defaultSearchListSize = sizeof(defaultSearchList) / sizeof(defaultSearchList[0]);

// --------------------------------------------------------------------------
// Lazy-load: fill _list from NVS or fall back to the built-in default.
// --------------------------------------------------------------------------
void DataSpaceList::ensureLoaded() {
    if (_loaded) return;

    std::unique_ptr<uint8_t[]> led     (new uint8_t[LED_SLOT_COUNT]);
    std::unique_ptr<String[]>  name    (new String[LED_SLOT_COUNT]);
    std::unique_ptr<String[]>  city    (new String[LED_SLOT_COUNT]);
    std::unique_ptr<bool[]>    disabled(new bool[LED_SLOT_COUNT]());
    int count = AppConfig::getInstance().loadSpaceMap(led.get(), name.get(), city.get(), disabled.get(), LED_SLOT_COUNT);

    if (count > 0) {
        _list.clear();
        for (int i = 0; i < count; i++) {
            _list.emplace_back(led[i], name[i], city[i], disabled[i]);
        }
        _loaded = true;
    } else {
        // Nothing in NVS — load compiled-in defaults and persist them
        // immediately so they survive reboot and the Reset button works.
        _list.clear();
        for (int i = 0; i < defaultSearchListSize; i++) {
            _list.push_back(defaultSearchList[i]);
        }
        _loaded = true;
        std::unique_ptr<uint8_t[]> dLed     (new uint8_t[LED_SLOT_COUNT]);
        std::unique_ptr<String[]>  dName    (new String[LED_SLOT_COUNT]);
        std::unique_ptr<String[]>  dCity    (new String[LED_SLOT_COUNT]);
        std::unique_ptr<bool[]>    dDisabled(new bool[LED_SLOT_COUNT]());
        for (int i = 0; i < defaultSearchListSize; i++) {
            dLed[i]      = (uint8_t)_list[i].getLED();
            dName[i]     = _list[i].getName();
            dCity[i]     = _list[i].city;
            dDisabled[i] = _list[i].disabled;
        }
        AppConfig::getInstance().saveSpaceMap(dLed.get(), dName.get(), dCity.get(), dDisabled.get(), defaultSearchListSize);
    }
}

// --------------------------------------------------------------------------
// Returns the LED index for the hackerspace with the given name.
// Returns -1 if the name is not found in the watch list.
// --------------------------------------------------------------------------
int DataSpaceList::getLEDforName(String name) {
    ensureLoaded();
    if (name.length() == 0) return -1;
    for (const auto& entry : _list) {
        if (entry.getName().length() == 0) continue; // empty slot = black LED, skip
        if (entry.getName() == name) {
            return entry.getLED();
        }
    }
    return -1;
}

int DataSpaceList::getNumberofSpacesonwatch() {
    ensureLoaded();
    int count = 0;
    for (const auto& entry : _list) {
        if (entry.getName().length() > 0) count++;
    }
    return count;
}

const std::vector<SpaceSearchList>& DataSpaceList::getList() {
    ensureLoaded();
    return _list;
}

void DataSpaceList::saveList(const std::vector<SpaceSearchList>& list) {
    _list = list;
    _loaded = true;
    int count = (int)list.size();
    if (count > LED_SLOT_COUNT) count = LED_SLOT_COUNT;
    std::unique_ptr<uint8_t[]> led     (new uint8_t[count]);
    std::unique_ptr<String[]>  name    (new String[count]);
    std::unique_ptr<String[]>  city    (new String[count]);
    std::unique_ptr<bool[]>    disabled(new bool[count]());
    for (int i = 0; i < count; i++) {
        led[i]      = (uint8_t)list[i].getLED();
        name[i]     = list[i].getName();
        city[i]     = list[i].city;
        disabled[i] = list[i].disabled;
    }
    AppConfig::getInstance().saveSpaceMap(led.get(), name.get(), city.get(), disabled.get(), count);
}

void DataSpaceList::resetToDefault() {
    // Rebuild _list directly from the compiled-in default table.
    _list.clear();
    for (int i = 0; i < defaultSearchListSize; i++) {
        _list.push_back(defaultSearchList[i]);
    }
    _loaded = true;
    // Clear NVS first so stale keys from a previously longer list cannot
    // survive, then write the defaults in a fresh open/close cycle.
    AppConfig::getInstance().resetSpaceMap();
    int count = (int)_list.size();
    std::unique_ptr<uint8_t[]> led     (new uint8_t[count]);
    std::unique_ptr<String[]>  name    (new String[count]);
    std::unique_ptr<String[]>  city    (new String[count]);
    std::unique_ptr<bool[]>    disabled(new bool[count]());
    for (int i = 0; i < count; i++) {
        led[i]      = (uint8_t)_list[i].getLED();
        name[i]     = _list[i].getName();
        city[i]     = _list[i].city;
        disabled[i] = _list[i].disabled;
    }
    AppConfig::getInstance().saveSpaceMap(led.get(), name.get(), city.get(), disabled.get(), count);
}