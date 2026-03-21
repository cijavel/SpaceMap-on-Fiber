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

private:
    WebClientHandler() = delete; // Purely static – prevent instantiation.

    static int           _lastHttpCode;
    static unsigned long _lastAttemptMs;

    // Add a new entry or update the status of an existing one in spaceStatusList.
    static void updateOrInsertStatus(std::vector<SpaceStatusList>& spaceStatusList,
                                     int ledIndex, const String& name, SpaceStatus status);
};

#endif // SPACE_API_ON_FIBER_WEBCLIENTHANDLER_H
