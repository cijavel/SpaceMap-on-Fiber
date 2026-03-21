#include "WebClientHandler.h"

// --------------------------------------------------------------------------
// Add a new Hackerspace or update the status of an existing one
// --------------------------------------------------------------------------
void WebClientHandler::modifyStatus(std::vector<SpaceStatusList> &spaStaVector,
                                    int led, const String &name, SpaceStatus status) {
    for (auto &element : spaStaVector) {
        if (element.getName() != name) continue;

        if (element.getStatus() != status) {
            element.setStatus(status);
            element.setlastChange(TimeHandler::localTime("%Y.%m.%d %H:%M"));
        }
        return; // found and (maybe) updated – done
    }
    // Not found – add as new entry
    spaStaVector.push_back({led, name, status, TimeHandler::localTime("%Y.%m.%d %H:%M")});
}

// --------------------------------------------------------------------------
// Download and parse the SpaceAPI JSON, then update the status vector
// --------------------------------------------------------------------------
void WebClientHandler::getSpaceStatus(std::vector<SpaceStatusList> &spaceStatusVector,
                                      const String &webpageout) {
    DataSpaceList &SpaceBase = DataSpaceList::getInstance();

    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure(); // No CA bundle on the device – acceptable for this use case

    http.begin(client, webpageout);
    http.useHTTP10(true); // Required for streamed/chunked reading

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.print(F("HTTP GET failed, code: "));
        Serial.println(httpCode);
        http.end();
        return;
    }

    Stream &payload = http.getStream();

    // Filter: only pull fields we actually need to reduce memory pressure
    StaticJsonDocument<128> filter;
    filter["space"]          = true;
    filter["state"]["open"]  = true;

    // Size budget: one parsed space entry is small; 4096 is comfortable for one
    // object but ArduinoJson streams one object at a time here, so this is fine.
    DynamicJsonDocument doc(4096);

    if (!payload.find("[")) { // JSON root must be an array
        Serial.println(F("SpaceAPI: expected JSON array, got something else"));
        http.end();
        return;
    }

    do {
        DeserializationError error = deserializeJson(doc, payload,
                                                     DeserializationOption::Filter(filter));
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            continue;
        }

        String spaceName = doc["space"].as<String>();
        int led = SpaceBase.getLEDforName(spaceName);
        if (led < 0) continue; // not a space we care about

        // doc["state"]["open"] can be true, false, or missing/null
        JsonVariant openField = doc["state"]["open"];
        switch (item.getStatus()) {
            case SpaceStatus::OPEN:    color = setBrightness(copen,    brightness); break;
            case SpaceStatus::CLOSED:  color = setBrightness(cclosed,  brightness); break;
            case SpaceStatus::UNKNOWN: color = setBrightness(cunknown, brightness); break;
            default:                   color = setBrightness(cblack,   brightness); break;
        }

        modifyStatus(spaceStatusVector, led, spaceName, status);

    } while (payload.findUntil(",", "]"));

    http.end();
}
