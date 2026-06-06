#include "TimeHandler.h"

// NTP configuration (file-local — internal linkage).
static constexpr const char* ntpServer         = "pool.ntp.org";
static constexpr long        gmtOffsetSec      = 0;
static constexpr int         daylightOffsetSec = 3600;
static const String          timezoneRule      = "CET-1CEST,M3.5.0,M10.5.0/3"; // POSIX timezone rule for Central Europe

// --------------------------------------------------------------------------
// Initialise the system clock via NTP and apply the POSIX timezone rule.
// --------------------------------------------------------------------------
void TimeHandler::initTime() {
    configTime(gmtOffsetSec, daylightOffsetSec, ntpServer);

    // Apply the timezone rule once. setenv() allocates on the heap, and the
    // localTime() function used to call it on every status update — that
    // caused steady heap fragmentation over weeks of uptime.
    setenv("TZ", timezoneRule.c_str(), 1);
    tzset();
}

// --------------------------------------------------------------------------
// Return the current local time as a string using the given strftime format.
// --------------------------------------------------------------------------
String TimeHandler::localTime(const String& format) {
    struct tm timeinfo{};
    char formattedTime[60];

    if (!getLocalTime(&timeinfo)) {
        return "TIME: Failed to obtain";
    }

    strftime(formattedTime, sizeof(formattedTime), format.c_str(), &timeinfo);
    return String(formattedTime);
}
