#ifndef SPACE_API_ON_FIBER_TIMEHANDLER_H
#define SPACE_API_ON_FIBER_TIMEHANDLER_H

#include <Arduino.h>
#include <ctime>

// Handles NTP synchronisation and local time formatting.
class TimeHandler {
public:
    // Synchronise system time with the NTP server. Call once in setup().
    static void initTime();

    // Return the current local time formatted according to the given strftime format string.
    // Returns an error string if the time has not yet been synchronised.
    static String localTime(const String& format);
};

#endif // SPACE_API_ON_FIBER_TIMEHANDLER_H
