#ifndef SPACE_API_ON_FIBER_WIFIHANDLER_H
#define SPACE_API_ON_FIBER_WIFIHANDLER_H

#include <WiFi.h> // please rename credentials_example.h to credentials.h
#include "Credentials.h"
#include "Configuration.h"


class WiFiHandler{
public:
    static void initWifi();
    static bool checkWifi(unsigned long currentSeconds);
private:
    static bool StatusCheck();
    static void ReStart();
};
#endif //SPACE_API_ON_FIBER_WIFIHANDLER_H
