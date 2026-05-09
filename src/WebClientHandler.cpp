#include "WebClientHandler.h"

int           WebClientHandler::_lastHttpCode  = 0;
unsigned long WebClientHandler::_lastAttemptMs = 0;

int  WebClientHandler::_lastFoundCount    = 0;
int  WebClientHandler::_lastParseErrors   = 0;
int  WebClientHandler::_lastTotalObjects  = 0;
int  WebClientHandler::_lastWatchListSize = 0;
std::vector<String> WebClientHandler::_lastUnmatchedNames;

WiFiClientSecure WebClientHandler::_sharedTlsClient;
bool             WebClientHandler::_tlsClientReady = false;

// --------------------------------------------------------------------------
// Add a new hackerspace entry or update the status of an existing one.
// The timestamp is only refreshed when the status actually changes.
// --------------------------------------------------------------------------
void WebClientHandler::updateOrInsertStatus(std::vector<SpaceStatusList>& spaceStatusList,
                                            int ledIndex, const String& name, SpaceStatus status) {
    for (auto& entry : spaceStatusList) {
        if (entry.getName() != name) continue;

        if (entry.getStatus() != status) {
            entry.setStatus(status);
            entry.setlastChange(TimeHandler::localTime("%Y.%m.%d %H:%M"));
        }
        return; // Found and (maybe) updated – done.
    }
    // Not in the list yet – add as a new entry.
    spaceStatusList.push_back({ledIndex, name, status, TimeHandler::localTime("%Y.%m.%d %H:%M")});
}

// --------------------------------------------------------------------------
// Download and parse the SpaceAPI JSON feed, then update the status vector.
// Streams the response one object at a time to keep memory usage low.
// --------------------------------------------------------------------------
void WebClientHandler::getSpaceStatus(std::vector<SpaceStatusList>& spaceStatusList,
                                      const String& spaceApiUrl) {
    // Reset parse counters for this run.
    _lastFoundCount    = 0;
    _lastParseErrors   = 0;
    _lastTotalObjects  = 0;
    _lastUnmatchedNames.clear();

    DataSpaceList& spaceDirectory = DataSpaceList::getInstance();

    // Snapshot watch list for unmatched-name tracking.
    // Empty entries (no name configured) are excluded from matching and
    // must never appear as "not found" in the status report.
    const auto& watchList = spaceDirectory.getList();
    _lastUnmatchedNames.reserve(watchList.size());
    std::vector<bool> matchedFlags(watchList.size(), false);

    int activeWatchCount = 0;
    for (const auto& entry : watchList) {
        if (entry.getName().length() > 0) activeWatchCount++;
    }
    _lastWatchListSize = activeWatchCount;

    // Configure the shared TLS client once. After this call, the mbedTLS
    // buffers stay alive for the lifetime of the program — no re-allocation
    // on every API fetch.
    if (!_tlsClientReady) {
        _sharedTlsClient.setInsecure(); // No CA bundle on the device – acceptable for this use case.
        _tlsClientReady = true;
    }

    HTTPClient http;
    http.begin(_sharedTlsClient, spaceApiUrl);
    http.useHTTP10(true); // Required for streamed / chunked reading.

    _lastAttemptMs = millis();
    int httpCode = http.GET();
    _lastHttpCode = httpCode;

    if (httpCode != HTTP_CODE_OK) {
        Serial.print(F("HTTP GET failed, code: "));
        Serial.println(httpCode);
        http.end();
        return;
    }

    Stream& responseStream = http.getStream();

    // Filter: pull only the fields we actually need to reduce memory pressure.
    // The SpaceAPI wraps all space data inside a "data" object:
    // [ { "url":"...", "valid":true, "data": { "space":"Name", "state":{"open":true} } }, ... ]
    JsonDocument filter;
    filter["data"]["space"]         = true;
    filter["data"]["state"]["open"] = true;

    // One parsed space object at a time.
    JsonDocument spaceDoc;

    if (!responseStream.find("[")) { // SpaceAPI root must be a JSON array.
        Serial.println(F("SpaceAPI: expected JSON array, got something else"));
        http.end();
        return;
    }

    do {
        DeserializationError parseError = deserializeJson(spaceDoc, responseStream,
                                                          DeserializationOption::Filter(filter));
        if (parseError) {
            _lastParseErrors++;
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(parseError.c_str());
            continue;
        }

        _lastTotalObjects++;
        String spaceName = spaceDoc["data"]["space"].as<String>();
        int ledIndex = spaceDirectory.getLEDforName(spaceName);
        if (ledIndex < 0) continue; // Not a space we track.

        // Mark this watch-list entry as matched.
        for (size_t wi = 0; wi < watchList.size(); wi++) {
            if (watchList[wi].getName() == spaceName) { matchedFlags[wi] = true; break; }
        }
        _lastFoundCount++;

        // The "open" field can be true, false, or absent/null.
        JsonVariant openField = spaceDoc["data"]["state"]["open"];
        SpaceStatus status;
        if (openField.is<bool>()) {
            status = openField.as<bool>() ? SpaceStatus::OPEN : SpaceStatus::CLOSED;
        } else {
            status = SpaceStatus::UNKNOWN;
        }

        updateOrInsertStatus(spaceStatusList, ledIndex, spaceName, status);

    } while (responseStream.findUntil(",", "]"));

    http.end();

    // Collect watch-list entries that never appeared in the API response.
    // Skip empty slots — they are intentionally unused and not an error.
    for (size_t wi = 0; wi < watchList.size(); wi++) {
        if (!matchedFlags[wi] && watchList[wi].getName().length() > 0) {
            _lastUnmatchedNames.push_back(watchList[wi].getName());
        }
    }
}