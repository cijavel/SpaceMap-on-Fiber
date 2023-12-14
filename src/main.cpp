#include <Arduino.h>

// _______________
// LED
// ---------------
// DATA -> 4

#include <vector>
#include <NeoPixelBus.h>

#include "Configuration.h"
#include "WiFiHandler.h"
#include "WebClientHandler.h"
#include "DataSpaceList.h"
#include "TimeHandler.h"
#include "DataStructure.h"
#include "NeoPixelLED.h"
#include "WebServerHandler.h"


#ifdef DEBUG
static void PrintRamUsage(unsigned long currentSeconds) {
    if (currentSeconds % interval_in_Seconds_RAMPrintout == 0) {
        Serial.print("Memory Usage: ");
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t maximumHeap = ESP.getHeapSize();
        uint32_t usedHeap = maximumHeap - freeHeap;
        Serial.print(usedHeap);
        Serial.print("b | ");
        Serial.print(maximumHeap);
        Serial.println("b");
    }
}
#endif

std::vector<SpaceStatusList> spacestatus;
unsigned long last = 0;

// --------------------------------------------------------------------------
// SETUP
// --------------------------------------------------------------------------
void setup() {
    delay(100);
    Serial.begin(BAUDRATE);
    Serial.println();
    WiFiHandler::initWifi();
    TimeHandler::initTime();
    NeoPixelLED &NeoLED = NeoPixelLED::getInstance();
    NeoLED.initLEDs();
    NeoLED.enumerateLEDs(500);
    WebServerHandler &webServer = WebServerHandler::getInstance();
    webServer.start();

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
    WebServerHandler &webServer = WebServerHandler::getInstance();


    unsigned long currentSeconds = millis() / 1000;
    #ifdef DEBUG
        if (currentSeconds != last) {
            Serial.print("Current loop second: ");
            Serial.println(currentSeconds);
            last = currentSeconds;
        }

        if (currentSeconds % interval_in_Seconds_Json == 0){    
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

        spacestatus = WebHandlerobj.getSpaceStatus(spacestatus, F(webpage_SpaceAPI), currentSeconds);

        NeoLED.updateLEDs(spacestatus, currentSeconds);
        webServer.setData(spacestatus, currentSeconds);
}
