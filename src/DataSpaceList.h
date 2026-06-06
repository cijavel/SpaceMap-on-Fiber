#ifndef SPACE_API_ON_FIBER_DATASPACELIST_H
#define SPACE_API_ON_FIBER_DATASPACELIST_H

#include <Arduino.h>
#include "Configuration.h"
#include "DataStructure.h"
#include <ArduinoJson.h>
#include <vector>
#include <freertos/semphr.h>

// Provides access to the list of tracked hackerspaces.
// On first use the list is loaded from NVS; if none is stored the built-in
// default from DataSpaceList.cpp is used as fallback.
class DataSpaceList {
private:
    DataSpaceList() : _loaded(false), _listMutex(xSemaphoreCreateMutex()) {}
    DataSpaceList(DataSpaceList const&) = delete;
    void operator=(DataSpaceList const&) = delete;

    bool _loaded;
    std::vector<SpaceSearchList> _list;

    // Guards every read/write access to _list. The list is read from the
    // loop task (LED update, API fetch) and written from the async_tcp task
    // (web UI save/reset) — without this they could race during a reallocation.
    SemaphoreHandle_t _listMutex;

    // Fills _list: tries NVS first, falls back to the compiled-in searchList[].
    // Must be called with _listMutex already held.
    void ensureLoadedLocked();

public:
    static DataSpaceList& getInstance() {
        static DataSpaceList instance;
        return instance;
    }

    // Returns the LED index for the given hackerspace name, or -1 if not found.
    int getLEDforName(const String& name);

    // Returns the total number of hackerspaces in the watch list.
    int getNumberofSpacesonwatch();

    // Returns a snapshot copy of the active list (for the web UI / LED update).
    // A copy is returned — not a reference — so the caller can iterate it
    // safely even if another task replaces the list in the meantime.
    std::vector<SpaceSearchList> getList();

    // Replaces the active list and persists it to NVS.
    void saveList(const std::vector<SpaceSearchList>& list);

    // Removes the NVS mapping and reloads from the built-in default.
    void resetToDefault();
};

#endif // SPACE_API_ON_FIBER_DATASPACELIST_H