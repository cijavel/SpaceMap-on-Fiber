#include "WiFiHandler.h"
#include "NeoPixelLED.h"

int WiFiHandler::failedReconnectCount = 0;

// --------------------------------------------------------------------------
// Connect to WiFi on startup.
// Retries up to WIFI_MAX_RETRIES times; reboots if all attempts fail.
// --------------------------------------------------------------------------
void WiFiHandler::initWifi() {
    WiFiClass::setHostname(DeviceName);
#ifdef DEBUG
    Serial.print("\nWIFI: Connecting to ");
    Serial.println(WIFI_SSID);
#endif

    int reconnectAttempts = 0;
    while (WiFiClass::status() != WL_CONNECTED && reconnectAttempts < WIFI_MAX_RETRIES) {
        WiFi.begin(WIFI_SSID, WIFI_PW);

        int connectionAttempts = 0;
        while (WiFiClass::status() != WL_CONNECTED && connectionAttempts < WIFI_CONNECT_TIMEOUT_STEPS) {
            delay(500);
            connectionAttempts++;
        }

        if (WiFiClass::status() != WL_CONNECTED) {
            reconnectAttempts++;
#ifdef DEBUG
            Serial.print("WIFI: retry ");
            Serial.print(reconnectAttempts);
            Serial.print(" of ");
            Serial.println(WIFI_MAX_RETRIES);
#endif
            WiFi.disconnect();
            delay(1000);
        }
    }

#ifdef DEBUG
    if (WiFiClass::status() == WL_CONNECTED) {
        Serial.println("WIFI: connected.");
        Serial.print("WIFI: IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("WIFI: not connected after all retries");
    }
#endif

    if (WiFiClass::status() != WL_CONNECTED) {
        Serial.println("WIFI: init failed, rebooting...");
        delay(1000);
        ESP.restart();
    }

    // Always print IP so the web UI can be reached even without DEBUG builds.
    Serial.print("WIFI: IP address: ");
    Serial.println(WiFi.localIP());

    // Start mDNS so the device is reachable as http://spacemap.local
    if (MDNS.begin(DeviceName)) {
        Serial.println("WIFI: mDNS started -> http://spacemap.local");
    }
}

// --------------------------------------------------------------------------
// Verify the connection and attempt a reconnect if needed.
// Increments the failure counter and reboots after WIFI_MAX_FAILED_RECONNECTS
// consecutive failures.
// Returns true if connected after this call.
// --------------------------------------------------------------------------
bool WiFiHandler::verifyAndReconnect() {
    wl_status_t status = WiFiClass::status();
    if (status != WL_CONNECTED) {
        Serial.print("WIFI: reconnecting");
        reconnect();
        status = WiFiClass::status();
        if (status != WL_CONNECTED) {
            failedReconnectCount++;
            Serial.print("WIFI: failed reconnects: ");
            Serial.println(failedReconnectCount);
            if (failedReconnectCount >= WIFI_MAX_FAILED_RECONNECTS) {
                Serial.println("WIFI: max reconnects reached, rebooting...");
                delay(1000);
                ESP.restart();
            }
        } else {
            failedReconnectCount = 0; // Reset counter on successful reconnect.
        }
    }
    return status == WL_CONNECTED;
}

// --------------------------------------------------------------------------
// Disconnect and re-establish the WiFi connection.
// --------------------------------------------------------------------------
void WiFiHandler::reconnect() {
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(WIFI_SSID, WIFI_PW);

    int connectionAttempts = 0;
    while (WiFiClass::status() != WL_CONNECTED && connectionAttempts < WIFI_CONNECT_TIMEOUT_STEPS) {
        delay(500);
        connectionAttempts++;
    }

#ifdef DEBUG
    if (WiFiClass::status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("WIFI: reconnected.");
        Serial.println(WiFi.localIP());
    }
#endif
}

// --------------------------------------------------------------------------
// Check connection health and reflect the result on the onboard LED.
// Call periodically from the main loop.
// --------------------------------------------------------------------------
bool WiFiHandler::checkWifi() {
    bool connected = WiFiHandler::verifyAndReconnect();

    if (connected) {
        neopixelWrite(RGB_BUILTIN, 0, ONBOARD_BRIGHTNESS, 0); // GREEN - connected
    } else {
        neopixelWrite(RGB_BUILTIN, ONBOARD_BRIGHTNESS, 0, 0); // RED - disconnected
    }
    return connected;
}
