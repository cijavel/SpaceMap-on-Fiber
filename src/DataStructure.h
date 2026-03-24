#include "arduino.h"

#ifndef SPACE_API_ON_DATASTRUCTURE_H
#define SPACE_API_ON_DATASTRUCTURE_H

enum class SpaceStatus { INIT, OPEN, CLOSED, UNKNOWN };

// --------------------------------------------------------------------------
// Describes one entry in the static list of hackerspaces to track:
// the LED index it maps to, its display name, and its city.
// --------------------------------------------------------------------------
struct SpaceSearchList {
    uint8_t ledIndex;
    String  name;
    String  city;
    bool    disabled;

    SpaceSearchList(uint8_t ledIndex, String name, String city, bool disabled = false) {
        this->ledIndex  = ledIndex;
        this->name      = name;
        this->city      = city;
        this->disabled  = disabled;
    }

    int getLED() const {
        return this->ledIndex;
    }

    String getName() const {
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

    SpaceStatusList(int ledIndex, String name, SpaceStatus status, String lastChange) {
        this->ledIndex    = ledIndex;
        this->name        = name;
        this->status      = status;
        this->lastChange  = lastChange;
    }

    int getLED() const {
        return this->ledIndex;
    }

    String getName() const {
        return this->name;
    }

    SpaceStatus getStatus() const {
        return this->status;
    }

    void setStatus(SpaceStatus status) {
        this->status = status;
    }

    String getlastChange() const {
        return this->lastChange;
    }

    void setlastChange(String lastChange) {
        this->lastChange = lastChange;
    }
};

#endif // SPACE_API_ON_DATASTRUCTURE_H
