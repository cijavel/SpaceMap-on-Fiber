
#include "WiFiHandler.h"
#include "NeoPixelLED.h"

// --------------------------------------------------------------------------
// initial Wifi
// --------------------------------------------------------------------------
void WiFiHandler::initWifi() {
    WiFiClass::setHostname(DeviceName);
    #ifdef DEBUG
        Serial.print("\nWIFI: Connecting to ");
        Serial.println(WIFI_SSID);
    #endif

    int retryCount = 0;
    while (WiFiClass::status() != WL_CONNECTED && retryCount < WIFI_MAX_RETRIES) {
        WiFi.begin(WIFI_SSID, WIFI_PW);
        int wifiWaitCount = 0;
        while (WiFiClass::status() != WL_CONNECTED && wifiWaitCount < WIFI_CONNECT_TIMEOUT_STEPS)
        {
            delay(500);
            wifiWaitCount++;
        }
        if (WiFiClass::status() != WL_CONNECTED) {
            retryCount++;
            #ifdef DEBUG
                Serial.print("WIFI: retry ");
                Serial.print(retryCount);
                Serial.print(" of ");
                Serial.println(WIFI_MAX_RETRIES);
            #endif
            WiFi.disconnect();
            delay(1000);
        }
    }

    #ifdef DEBUG
        if (WiFiClass::status() == WL_CONNECTED)
        {
            Serial.println("WIFI: connected.");
            Serial.print("WIFI: IP address: ");
            Serial.println(WiFi.localIP());
        }
        else
        {
            Serial.println("WIFI: not connected after all retries");
        }
    #endif
}

// --------------------------------------------------------------------------
// check wifi status
// --------------------------------------------------------------------------
bool WiFiHandler::StatusCheck()
{
    wl_status_t status = WiFiClass::status();
    if (status != WL_CONNECTED)
    {
        Serial.print("WIFI: reconnecting");
        ReStart();
        status = WiFiClass::status();  // Status nach Reconnect neu lesen
    }
    return status == WL_CONNECTED;
}

// --------------------------------------------------------------------------
// restart wifi
// --------------------------------------------------------------------------
void WiFiHandler::ReStart()
{
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(WIFI_SSID, WIFI_PW);
    int wifiWaitCount = 0;
    while (WiFiClass::status() != WL_CONNECTED && wifiWaitCount < WIFI_CONNECT_TIMEOUT_STEPS)
    {
        delay(500);
        wifiWaitCount++;
    }
#ifdef DEBUG
    if (WiFiClass::status() == WL_CONNECTED)
    {
        Serial.println();
        Serial.println("WiFi connected");

        // Print the IP address
        Serial.println(WiFi.localIP());
    }
#endif
}

// --------------------------------------------------------------------------
// checkout wifi in interval
// --------------------------------------------------------------------------
bool WiFiHandler::checkWifi() {
    WiFiHandler::StatusCheck();

    if (WiFiClass::status() == WL_CONNECTED)
    {
        neopixelWrite(RGB_BUILTIN ,0,ONBOARD_BRIGHTNESS,0); // GREEN
    }
    else
    {
        neopixelWrite(RGB_BUILTIN ,ONBOARD_BRIGHTNESS,0,0); // RED
    }
    return WiFiClass::status() == WL_CONNECTED;
}

