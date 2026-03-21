#include "WebClientHandler.h"

int           WebClientHandler::_lastHttpCode  = 0;
unsigned long WebClientHandler::_lastAttemptMs = 0;

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
    DataSpaceList& spaceDirectory = DataSpaceList::getInstance();

    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure(); // No CA bundle on the device – acceptable for this use case.

    http.begin(client, spaceApiUrl);
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
    StaticJsonDocument<128> filter;
    filter["space"]         = true;
    filter["state"]["open"] = true;

    // One parsed space object at a time – 4 KB is comfortable for a single entry.
    DynamicJsonDocument spaceDoc(4096);

    if (!responseStream.find("[")) { // SpaceAPI root must be a JSON array.
        Serial.println(F("SpaceAPI: expected JSON array, got something else"));
        http.end();
        return;
    }

    do {
        DeserializationError parseError = deserializeJson(spaceDoc, responseStream,
                                                          DeserializationOption::Filter(filter));
        if (parseError) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(parseError.c_str());
            continue;
        }

        String spaceName = spaceDoc["space"].as<String>();
        int ledIndex = spaceDirectory.getLEDforName(spaceName);
        if (ledIndex < 0) continue; // Not a space we track.

        // The "open" field can be true, false, or absent/null.
        JsonVariant openField = spaceDoc["state"]["open"];
        SpaceStatus status;
        if (openField.is<bool>()) {
            status = openField.as<bool>() ? SpaceStatus::OPEN : SpaceStatus::CLOSED;
        } else {
            status = SpaceStatus::UNKNOWN;
        }

        updateOrInsertStatus(spaceStatusList, ledIndex, spaceName, status);

    } while (responseStream.findUntil(",", "]"));

    http.end();
}
