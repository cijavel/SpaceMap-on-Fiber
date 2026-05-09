#ifndef SPACE_API_ON_FIBER_WEBCLIENTHANDLER_H
#define SPACE_API_ON_FIBER_WEBCLIENTHANDLER_H

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "Configuration.h"
#include <ArduinoJson.h>
#include "DataStructure.h"
#include <vector>
#include "DataSpaceList.h"
#include "TimeHandler.h"

// Fetches and parses the SpaceAPI JSON feed.
// All methods are static – this class is never instantiated.
class WebClientHandler {
public:
    // Download the SpaceAPI endpoint and update spaceStatusList with current open/closed states.
    static void getSpaceStatus(std::vector<SpaceStatusList>& spaceStatusList,
                               const String& spaceApiUrl);

    // Returns the HTTP status code of the most recent API fetch (0 = never fetched).
    static int  getLastHttpCode()   { return _lastHttpCode; }

    // Returns the millis() timestamp of the most recent fetch attempt (0 = never).
    static unsigned long getLastAttemptMs() { return _lastAttemptMs; }

    // Parse-result counters from the most recent successful fetch.
    static int getLastFoundCount()   { return _lastFoundCount; }
    static int getLastParseErrors()  { return _lastParseErrors; }
    static int getLastTotalObjects() { return _lastTotalObjects; }
    static int getLastWatchListSize(){ return _lastWatchListSize; }

    // Names from the watch list that were NOT present in the last API response.
    // Only populated when HTTP 200 was received; empty if watch list is empty.
    static const std::vector<String>& getLastUnmatchedNames() { return _lastUnmatchedNames; }

private:
    WebClientHandler() = delete; // Purely static – prevent instantiation.

    static int           _lastHttpCode;
    static unsigned long _lastAttemptMs;

    static int  _lastFoundCount;
    static int  _lastParseErrors;
    static int  _lastTotalObjects;
    static int  _lastWatchListSize;
    static std::vector<String> _lastUnmatchedNames;

    // Add a new entry or update the status of an existing one in spaceStatusList.
    static void updateOrInsertStatus(std::vector<SpaceStatusList>& spaceStatusList,
                                     int ledIndex, const String& name, SpaceStatus status);

    // Drop entries whose space name is no longer in the watch list, and
    // refresh the LED index of the remaining entries to match the current
    // SpaceMap. Prevents stale ledIndex values after the user edits the
    // mapping in the web UI.
    static void synchronizeStatusListWithMapping(std::vector<SpaceStatusList>& spaceStatusList,
                                                 DataSpaceList& spaceDirectory);
};

#endif // SPACE_API_ON_FIBER_WEBCLIENTHANDLER_H
