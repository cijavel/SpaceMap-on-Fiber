#include <Arduino.h>

// _______________
// LED
// ---------------
// DATA -> 4

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
#endif

std::vector<SpaceStatusList> spacestatus;
unsigned long lastApiCall = 0;
unsigned long lastWifiCheck = 0;
unsigned long lastLedUpdate = 0;
#ifdef DEBUG
unsigned long lastRamPrint = 0;
#endif

// --------------------------------------------------------------------------
// SETUP
// --------------------------------------------------------------------------
void setup() {


    delay(100);
    Serial.begin(BAUDRATE);
    Serial.println();
    Serial.print("\nINITIAL");
    WiFiHandler::initWifi();
    TimeHandler::initTime();
    NeoPixelLED &NeoLED = NeoPixelLED::getInstance();
    NeoLED.initLEDs();
    NeoLED.enumerateLEDs(500);

    #ifdef RGB_BUILTIN
        digitalWrite(RGB_BUILTIN, LOW);    // Turn the RGB LED off. Turn onboard LED off. HIGH to turn on
    #endif
    
}

// --------------------------------------------------------------------------
// LOOP
// --------------------------------------------------------------------------
void loop() {
    WebClientHandler &WebHandlerobj= WebClientHandler::getInstance();
    NeoPixelLED &NeoLED = NeoPixelLED::getInstance();


    unsigned long currentSeconds = millis() / 1000;

    if (currentSeconds - lastWifiCheck >= interval_in_Seconds_WiFiCheck) {
        WiFiHandler::checkWifi();
        lastWifiCheck = currentSeconds;
    }
    
    #ifdef DEBUG
    if (currentSeconds - lastRamPrint >= interval_in_Seconds_RAMPrintout) {
        PrintRamUsage();
        lastRamPrint = currentSeconds;
    }
    if (currentSeconds - lastApiCall >= interval_in_Seconds_api){
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
    if (currentSeconds - lastApiCall >= interval_in_Seconds_api) {
        neopixelWrite(RGB_BUILTIN, 0, 0, ONBOARD_BRIGHTNESS); // BLUE – API call running
        WebHandlerobj.getSpaceStatus(spacestatus, F(webpage_SpaceAPI));
        currentSeconds = millis() / 1000;
        lastApiCall = currentSeconds;
        lastLedUpdate = currentSeconds;
        if (WiFiClass::status() == WL_CONNECTED) {
            neopixelWrite(RGB_BUILTIN, 0, ONBOARD_BRIGHTNESS, 0); // GREEN
        } else {
            neopixelWrite(RGB_BUILTIN, ONBOARD_BRIGHTNESS, 0, 0); // RED
        }
    }
    if (currentSeconds - lastLedUpdate >= interval_in_Seconds_LEDs) {
        NeoLED.updateLEDs(spacestatus);
        lastLedUpdate = currentSeconds;
    }
}
