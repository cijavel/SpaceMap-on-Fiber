
#include "WiFiHandler.h"
#include "NeoPixelLED.h"

// --------------------------------------------------------------------------
// initial Wifi
// --------------------------------------------------------------------------
void WiFiHandler::initWifi() {
    int wifiWaitCount = 0;
    WiFiClass::setHostname(DeviceName); //maybe cstr
    #ifdef DEBUG
        Serial.print("\nWIFI: Connecting to ");
        Serial.println(WIFI_SSID);
    #endif

    WiFi.begin(WIFI_SSID, WIFI_PW);

    while (WiFiClass::status() != WL_CONNECTED && wifiWaitCount < 20)
    {
        delay(250);
        wifiWaitCount++;
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
            Serial.println("WIFI: not connected");
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
    }
    return status == WL_CONNECTED;
}

// --------------------------------------------------------------------------
// restart wifi
// --------------------------------------------------------------------------
void WiFiHandler::ReStart()
{
#ifdef DEBUG
    // Connect to WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);
#endif
    WiFi.begin(WIFI_SSID, WIFI_PW);
    int wifiWaitCount = 0;
    while (WiFiClass::status() != WL_CONNECTED && wifiWaitCount < 20)
    {
        delay(250);
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
bool WiFiHandler::checkWifi(unsigned long currentSeconds) {
    if (currentSeconds % interval_in_Seconds_WiFiCheck == 0) {
        WiFiHandler::StatusCheck();

        if (WiFiClass::status() == WL_CONNECTED)
        {
            neopixelWrite(RGB_BUILTIN ,0,ONBOARD_BRIGHTNESS,0); // GREEN
        }
        else
        {
            neopixelWrite(RGB_BUILTIN ,ONBOARD_BRIGHTNESS,0,0); // RED
        }
        return WL_CONNECTED;
    }
    return false;
}

