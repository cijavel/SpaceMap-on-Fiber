#ifndef SPACE_API_ON_FIBER_TIMEHANDLER_H
#define SPACE_API_ON_FIBER_TIMEHANDLER_H

#include <Arduino.h>
#include <ctime>


class TimeHandler {
private:


public:
    static String localTime(const String& format);
    static void initTime();
};
#endif //SPACE_API_ON_FIBER_TIMEHANDLER_H