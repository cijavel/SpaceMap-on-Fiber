#include "TimeHandler.h"


// --------------------------------------------------------------------------
// time function configuration
// --------------------------------------------------------------------------
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;
const String timezone = "CET-1CEST,M3.5.0,M10.5.0/3";

// --------------------------------------------------------------------------
// initial time function
// --------------------------------------------------------------------------
void  TimeHandler::initTime() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

// --------------------------------------------------------------------------
// get time for the given format
// --------------------------------------------------------------------------
String TimeHandler::localTime(const String& format) {
    struct tm timeinfo{};

    String time = "";
    char toutp[60];
    setenv("TZ", timezone.c_str(), 1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
    tzset();

    if (!getLocalTime(&timeinfo)) {
        time = "TIME: Failed to obtain";
    } else {
        strftime(toutp, sizeof(toutp), format.c_str(), &timeinfo);
        time = String(toutp);
    }
    return time;
}