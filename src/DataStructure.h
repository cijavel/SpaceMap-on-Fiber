#include "arduino.h"

#ifndef SPACE_API_ON_DATASTRUCTURE_H
#define SPACE_API_ON_DATASTRUCTURE_H

enum SpaceStatus {INIT, OPEN, CLOSED, UNKNOWN};

struct SpaceSearchList {
        uint8_t led;
        String name;

        SpaceSearchList(uint8_t led, String name) {
            this->led = led;
            this->name = name;
        }
        int getLED() const {
            return this->led;
        }

        String getName() const {
            return this->name;
        }
};




struct SpaceStatusList {
        int led;
        String name;
        SpaceStatus status;
        String lastChange;

        SpaceStatusList(int led, String name, SpaceStatus status, String lastchange) {
            this->led = led;
            this->name = name;
            this->status = status;
            this->lastChange =lastchange;
        }
        int getLED() const {
            return this->led;
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

        void setlastChange(String lastchange) {
            this->lastChange = lastchange;
        }
};

#endif // SPACE_API_ON_DATASTRUCTURE_H
