#ifndef SPACE_API_ON_DATASTRUCTURE_H
#define SPACE_API_ON_DATASTRUCTURE_H

#include <Arduino.h>
#include <utility>   // std::move

enum class SpaceStatus { init, open, closed, unknown };

// --------------------------------------------------------------------------
// Describes one entry in the static list of hackerspaces to track:
// the LED index it maps to, its display name, and its city.
// --------------------------------------------------------------------------
struct SpaceSearchList {
    uint8_t ledIndex;
    String  name;
    String  city;
    bool    disabled;

    SpaceSearchList(uint8_t ledIndex, String name, String city, bool disabled = false)
        : ledIndex(ledIndex), name(std::move(name)), city(std::move(city)), disabled(disabled) {}

    int getLED() const {
        return this->ledIndex;
    }

    const String& getName() const {
        return this->name;
    }

    bool isDisabled() const {
        return this->disabled;
    }
};

// --------------------------------------------------------------------------
// Holds the live status of a tracked hackerspace:
// which LED it drives, its name, current open/closed status,
// and the timestamp of the last status change.
// --------------------------------------------------------------------------
struct SpaceStatusList {
    int         ledIndex;
    String      name;
    SpaceStatus status;
    String      lastChange;

    SpaceStatusList(int ledIndex, String name, SpaceStatus status, String lastChange)
        : ledIndex(ledIndex), name(std::move(name)), status(status), lastChange(std::move(lastChange)) {}

    int getLED() const {
        return this->ledIndex;
    }

    const String& getName() const {
        return this->name;
    }

    SpaceStatus getStatus() const {
        return this->status;
    }

    void setStatus(SpaceStatus status) {
        this->status = status;
    }

    const String& getlastChange() const {
        return this->lastChange;
    }

    void setlastChange(String lastChange) {
        this->lastChange = lastChange;
    }
};

#endif // SPACE_API_ON_DATASTRUCTURE_H
