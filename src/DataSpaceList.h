#ifndef SPACE_API_ON_FIBER_DATASPACELIST_H
#define SPACE_API_ON_FIBER_DATASPACELIST_H

#include <Arduino.h>
#include "DataStructure.h"
#include <ArduinoJson.h>
#include <vector>

// Maximum number of LED<->Hackerspace mappings supported at runtime.
#define SPACEMAP_MAX_ENTRIES 64

// Provides access to the list of tracked hackerspaces.
// On first use the list is loaded from NVS; if none is stored the built-in
// default from DataSpaceList.cpp is used as fallback.
class DataSpaceList {
private:
    DataSpaceList() : _loaded(false) {}
    DataSpaceList(DataSpaceList const&);
    void operator=(DataSpaceList const&);

    bool _loaded;
    std::vector<SpaceSearchList> _list;

    // Fills _list: tries NVS first, falls back to the compiled-in searchList[].
    void ensureLoaded();

public:
    static DataSpaceList& getInstance() {
        static DataSpaceList instance;
        return instance;
    }

    // Returns the LED index for the given hackerspace name, or -1 if not found.
    int getLEDforName(String name);

    // Returns the total number of hackerspaces in the watch list.
    int getNumberofSpacesonwatch();

    // Returns a read-only reference to the active list (for the web UI).
    const std::vector<SpaceSearchList>& getList();

    // Replaces the active list and persists it to NVS.
    void saveList(const std::vector<SpaceSearchList>& list);

    // Removes the NVS mapping and reloads from the built-in default.
    void resetToDefault();
};

#endif // SPACE_API_ON_FIBER_DATASPACELIST_H