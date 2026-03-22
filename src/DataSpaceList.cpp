#include "DataSpaceList.h"
#include "AppConfig.h"

// --------------------------------------------------------------------------
// Built-in default list of hackerspaces.
// Each entry maps a LED strip index to a space name and city.
// This list is used as the factory default when no custom mapping is stored.
// --------------------------------------------------------------------------
static const SpaceSearchList defaultSearchList[] = {
    { 0, "OpenLab Augsburg"                  , "Augsburg"},
    { 1, "IT-Syndikat"                       , "Innsbruck"},
    { 2, "MuCCC"                             , "Munich"},
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

    uint8_t led[SPACEMAP_MAX_ENTRIES];
    String  name[SPACEMAP_MAX_ENTRIES];
    String  city[SPACEMAP_MAX_ENTRIES];
    int count = AppConfig::getInstance().loadSpaceMap(led, name, city, SPACEMAP_MAX_ENTRIES);

    if (count > 0) {
        _list.clear();
        for (int i = 0; i < count; i++) {
            _list.emplace_back(led[i], name[i], city[i]);
        }
    } else {
        _list.clear();
        for (int i = 0; i < defaultSearchListSize; i++) {
            _list.push_back(defaultSearchList[i]);
        }
    }
    _loaded = true;
}

// --------------------------------------------------------------------------
// Returns the LED index for the hackerspace with the given name.
// Returns -1 if the name is not found in the watch list.
// --------------------------------------------------------------------------
int DataSpaceList::getLEDforName(String name) {
    ensureLoaded();
    for (const auto& entry : _list) {
        if (entry.getName() == name) {
            return entry.getLED();
        }
    }
    return -1;
}

int DataSpaceList::getNumberofSpacesonwatch() {
    ensureLoaded();
    return (int)_list.size();
}

const std::vector<SpaceSearchList>& DataSpaceList::getList() {
    ensureLoaded();
    return _list;
}

void DataSpaceList::saveList(const std::vector<SpaceSearchList>& list) {
    _list = list;
    _loaded = true;
    uint8_t led[SPACEMAP_MAX_ENTRIES];
    String  name[SPACEMAP_MAX_ENTRIES];
    String  city[SPACEMAP_MAX_ENTRIES];
    int count = (int)list.size();
    if (count > SPACEMAP_MAX_ENTRIES) count = SPACEMAP_MAX_ENTRIES;
    for (int i = 0; i < count; i++) {
        led[i]  = (uint8_t)list[i].getLED();
        name[i] = list[i].getName();
        city[i] = list[i].city;
    }
    AppConfig::getInstance().saveSpaceMap(led, name, city, count);
}

void DataSpaceList::resetToDefault() {
    // Clear NVS and rebuild _list from the compiled-in default table.
    AppConfig::getInstance().resetSpaceMap();
    _loaded = false;
    _list.clear();
    for (int i = 0; i < defaultSearchListSize; i++) {
        _list.push_back(defaultSearchList[i]);
    }
    _loaded = true;
    // Persist the default list explicitly so NVS is never left empty.
    uint8_t led[SPACEMAP_MAX_ENTRIES];
    String  name[SPACEMAP_MAX_ENTRIES];
    String  city[SPACEMAP_MAX_ENTRIES];
    int count = (int)_list.size();
    for (int i = 0; i < count; i++) {
        led[i]  = (uint8_t)_list[i].getLED();
        name[i] = _list[i].getName();
        city[i] = _list[i].city;
    }
    AppConfig::getInstance().saveSpaceMap(led, name, city, count);
}