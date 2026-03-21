#ifndef SPACE_API_ON_FIBER_DATASPACELIST_H
#define SPACE_API_ON_FIBER_DATASPACELIST_H

#include <Arduino.h>
#include "DataStructure.h"
#include <ArduinoJson.h>

// Provides access to the static list of tracked hackerspaces (defined in DataSpaceList.cpp).
// Use getLEDforName() to resolve a space name to its LED strip index.
class DataSpaceList {
private:
    DataSpaceList() {};
    DataSpaceList(DataSpaceList const&);
    void operator=(DataSpaceList const&);

public:
    static DataSpaceList& getInstance() {
        static DataSpaceList instance;
        return instance;
    }

    // Returns the LED index for the given hackerspace name, or -1 if not found.
    int getLEDforName(String name);

    // Returns the total number of hackerspaces in the watch list.
    int getNumberofSpacesonwatch();
};

#endif // SPACE_API_ON_FIBER_DATASPACELIST_H
