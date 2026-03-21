#ifndef SPACE_API_ON_FIBER_WIFIHANDLER_H
#define SPACE_API_ON_FIBER_WIFIHANDLER_H

#include <WiFi.h> // Rename Credentials_example.h to Credentials.h before building.
#include "Credentials.h"
#include "Configuration.h"
#include <ESPmDNS.h>

// Manages WiFi connection lifecycle: initial connect, periodic health checks,
// and automatic reboot after too many consecutive failures.
class WiFiHandler {
public:
    // Connect to WiFi on startup. Reboots if no connection is established
    // within WIFI_MAX_RETRIES attempts.
    static void initWifi();

    // Check connection health and update the onboard LED accordingly.
    // Returns true if connected. Call periodically from the main loop.
    static bool checkWifi();

private:
    // Verify connection status and reconnect if necessary.
    // Increments the failed-reconnect counter and reboots if WIFI_MAX_FAILED_RECONNECTS is reached.
    static bool verifyAndReconnect();

    // Disconnect and attempt a fresh WiFi connection.
    static void reconnect();

    // Counts consecutive failed reconnect attempts; reset to 0 on success.
    static int failedReconnectCount;
};

#endif // SPACE_API_ON_FIBER_WIFIHANDLER_H
