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
#include "AppConfig.h"
#include "SettingsWebServer.h"

#ifdef DEBUG
    static void PrintRamUsage() {
        Serial.print("Memory Usage: ");
        uint32_t freeHeap    = ESP.getFreeHeap();
        uint32_t totalHeap   = ESP.getHeapSize();
        uint32_t usedHeap    = totalHeap - freeHeap;
        Serial.print(usedHeap);
        Serial.print("b | ");
        Serial.print(totalHeap);
        Serial.println("b");
    }

    static void PrintSpaceStatus(const std::vector<SpaceStatusList>& spaceStatusList) {
        Serial.println("Space Status:");
        for (const auto& entry : spaceStatusList) {
            Serial.print("led: ");
            Serial.print(entry.getLED());
            Serial.print(", name: ");
            Serial.print(entry.getName());
            Serial.print(", status: ");
            Serial.print(String(entry.getStatus()));
            Serial.print(", last: ");
            Serial.println(String(entry.getlastChange()));
        }
    }
#endif

// Holds the current open/closed status for each tracked hackerspace.
std::vector<SpaceStatusList> spaceStatusList;

// Timestamps in milliseconds for interval tracking.
// Using unsigned long and subtraction (now - last) handles millis() overflow
// correctly after ~49 days because unsigned wraparound gives the right delta.
unsigned long lastApiCall    = 0;
unsigned long lastWifiCheck  = 0;
unsigned long lastLedUpdate  = 0;
#ifdef DEBUG
unsigned long lastRamPrint   = 0;
#endif

// Interval for RAM debug printout (compile-time constant, not user-configurable).
#ifdef DEBUG
static constexpr unsigned long MS_RAM_PRINT = (unsigned long)interval_in_Seconds_RAMPrintout * 1000UL;
#endif

// --------------------------------------------------------------------------
// SETUP
// --------------------------------------------------------------------------
void setup() {
    delay(100);
    Serial.begin(BAUDRATE);
    Serial.println();
    Serial.println("\nINITIAL");

    // Load runtime configuration from NVS (defaults from Configuration.h).
    AppConfig::getInstance().load();

    WiFiHandler::initWifi();
    TimeHandler::initTime();

    // Start the web settings server on port 80.
    SettingsWebServer::getInstance().begin();

    NeoPixelLED &neoLED = NeoPixelLED::getInstance();
    neoLED.initLEDs();
    neoLED.enumerateLEDs(5000);

    // Fetch space status once immediately so the web UI shows data from the
    // very first page load instead of waiting for the first 120-second poll.
    neopixelWrite(RGB_BUILTIN, 0, 0, AppConfig::getInstance().getOnboardBrightness()); // BLUE
    WebClientHandler::getSpaceStatus(spaceStatusList, AppConfig::getInstance().getSpaceApiUrl());
    lastApiCall = millis();

    if (WiFiClass::status() == WL_CONNECTED) {
        neopixelWrite(RGB_BUILTIN, 0, AppConfig::getInstance().getOnboardBrightness(), 0); // GREEN
    } else {
        neopixelWrite(RGB_BUILTIN, AppConfig::getInstance().getOnboardBrightness(), 0, 0); // RED
    }

    #ifdef RGB_BUILTIN
        digitalWrite(RGB_BUILTIN, LOW);
    #endif
}

// --------------------------------------------------------------------------
// LOOP
// --------------------------------------------------------------------------
void loop() {
    NeoPixelLED &neoLED = NeoPixelLED::getInstance();

    // Use millis() directly; unsigned subtraction handles the ~49-day overflow.
    unsigned long now = millis();

    // --- WiFi watchdog ---
    if (now - lastWifiCheck >= AppConfig::getInstance().getIntervalWifiCheck() * 1000UL) {
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
    if (now - lastApiCall >= AppConfig::getInstance().getIntervalApi() * 1000UL) {
        #ifdef DEBUG
            PrintSpaceStatus(spaceStatusList);
        #endif

        const uint8_t onboardBrightness = AppConfig::getInstance().getOnboardBrightness();
        neopixelWrite(RGB_BUILTIN, 0, 0, onboardBrightness); // BLUE - API call running
        WebClientHandler::getSpaceStatus(spaceStatusList, AppConfig::getInstance().getSpaceApiUrl());

        // Refresh 'now' after the (potentially slow) HTTP call.
        now            = millis();
        lastApiCall    = now;
        lastLedUpdate  = now;

        if (WiFiClass::status() == WL_CONNECTED) {
            neopixelWrite(RGB_BUILTIN, 0, onboardBrightness, 0); // GREEN - connected
        } else {
            neopixelWrite(RGB_BUILTIN, onboardBrightness, 0, 0); // RED - disconnected
        }
    }

    // --- LED update ---
    if (now - lastLedUpdate >= AppConfig::getInstance().getIntervalLEDs() * 1000UL) {
        lastLedUpdate = now;
        neoLED.updateLEDs(spaceStatusList);
    }
}
