#include <Arduino.h>

#include <vector>
#include <NeoPixelBus.h>

#include "Configuration.h"
#include "WiFiHandler.h"
#include <WiFi.h>
#include "WebClientHandler.h"
#include "DataSpaceList.h"
#include "TimeHandler.h"
#include "DataStructure.h"
#include "NeoPixelLED.h"

#ifdef DEBUG
    static void PrintRamUsage() {
        Serial.print("Memory Usage: ");
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t maximumHeap = ESP.getHeapSize();
        uint32_t usedHeap = maximumHeap - freeHeap;
        Serial.print(usedHeap);
        Serial.print("b | ");
        Serial.print(maximumHeap);
        Serial.println("b");
    }

    static void PrintSpaceStatus(const std::vector<SpaceStatusList>& spacestatus) {
        Serial.println("Space Status:");
        for (const auto& data : spacestatus) {
            Serial.print("led: ");
            Serial.print(data.getLED());
            Serial.print(", name: ");
            Serial.print(data.getName());
            Serial.print(", status: ");
            Serial.print(String(data.getStatus()));
            Serial.print(", last: ");
            Serial.println(String(data.getlastChange()));
        }
    }
#endif

std::vector<SpaceStatusList> spacestatus;

// Timestamps in milliseconds.
// Using unsigned long and subtraction (now - last) handles millis() overflow
// correctly after ~49 days because unsigned wraparound gives the right delta.
unsigned long lastApiCall   = 0;
unsigned long lastWifiCheck = 0;
unsigned long lastLedUpdate = 0;
#ifdef DEBUG
unsigned long lastRamPrint  = 0;
#endif

// Convert interval constants (seconds) to milliseconds once at compile time.
static constexpr unsigned long MS_WIFI_CHECK = (unsigned long)interval_in_Seconds_WiFiCheck  * 1000UL;
static constexpr unsigned long MS_LED_UPDATE = (unsigned long)interval_in_Seconds_LEDs       * 1000UL;
static constexpr unsigned long MS_API_CALL   = (unsigned long)interval_in_Seconds_api        * 1000UL;
#ifdef DEBUG
static constexpr unsigned long MS_RAM_PRINT  = (unsigned long)interval_in_Seconds_RAMPrintout * 1000UL;
#endif

// --------------------------------------------------------------------------
// SETUP
// --------------------------------------------------------------------------
void setup() {
    delay(100);
    Serial.begin(BAUDRATE);
    Serial.println();
    Serial.println("\nINITIAL");

    WiFiHandler::initWifi();
    TimeHandler::initTime();

    NeoPixelLED &NeoLED = NeoPixelLED::getInstance();
    NeoLED.initLEDs();
    NeoLED.enumerateLEDs(500);

    #ifdef RGB_BUILTIN
        digitalWrite(RGB_BUILTIN, LOW);
    #endif
}

// --------------------------------------------------------------------------
// LOOP
// --------------------------------------------------------------------------
void loop() {
    NeoPixelLED &NeoLED = NeoPixelLED::getInstance();

    // Use millis() directly; unsigned subtraction handles the ~49-day overflow.
    unsigned long now = millis();

    // --- WiFi watchdog ---
    if (now - lastWifiCheck >= MS_WIFI_CHECK) {
        lastWifiCheck = now;
        WiFiHandler::checkWifi();
    }

    #ifdef DEBUG
    // --- RAM printout ---
    if (now - lastRamPrint >= MS_RAM_PRINT) {
        lastRamPrint = now;
        PrintRamUsage();
    }
    #endif

    // --- API fetch ---
    if (now - lastApiCall >= MS_API_CALL) {
        #ifdef DEBUG
            PrintSpaceStatus(spacestatus);
        #endif

        neopixelWrite(RGB_BUILTIN, 0, 0, ONBOARD_BRIGHTNESS); // BLUE – API call running
        WebClientHandler::getSpaceStatus(spacestatus, webpage_SpaceAPI);

        // Refresh 'now' after the (potentially slow) HTTP call.
        now           = millis();
        lastApiCall   = now;
        lastLedUpdate = now;

        if (WiFiClass::status() == WL_CONNECTED) {
            neopixelWrite(RGB_BUILTIN, 0, ONBOARD_BRIGHTNESS, 0); // GREEN
        } else {
            neopixelWrite(RGB_BUILTIN, ONBOARD_BRIGHTNESS, 0, 0); // RED
        }
    }

    // --- LED update ---
    if (now - lastLedUpdate >= MS_LED_UPDATE) {
        lastLedUpdate = now;
        NeoLED.updateLEDs(spacestatus);
    }
}
