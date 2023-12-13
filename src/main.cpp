#include <Arduino.h>

// _______________
// LED
// ---------------
// DATA -> 4

// https://stackoverflow.com/questions/8534526/how-to-initialize-an-array-of-struct-in-c


#include "Configuration.h"
#include "WiFiHandler.h"
#include "WebClientHandler.h"
#include "DataSpaceApi.h"
#include "TimeHandler.h"
#include "DataStructure.h"
#include <vector>


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




void setup() {
    delay(100);
    Serial.begin(BAUDRATE);
    Serial.println();
    WiFiHandler::initWifi();
    TimeHandler::initTime();
    DataSpaceApi &DataofSpaceApi = DataSpaceApi::getInstance();

    #ifdef RGB_BUILTIN
      digitalWrite(RGB_BUILTIN, LOW);    // Turn the RGB LED off. Turn onboard LED off. HIGH to turn on
      //neopixelWrite(RGB_BUILTIN,5,0,0); // Red
      //delay(1000);
    #endif
    
}

unsigned long last = 0;

void loop() {

    DataSpaceApi &DataofSpaceApi = DataSpaceApi::getInstance();
    WebClientHandler &WebHandlerobj= WebClientHandler::getInstance();

    unsigned long currentSeconds = millis() / 1000;
    #ifdef DEBUG
        if (currentSeconds != last) {
            Serial.print("Current loop second: ");
            Serial.println(currentSeconds);
            last = currentSeconds;
        }
    #endif


        std::vector<SpaceStatusList> spacestatus = WebHandlerobj.getSpaceStatus(F(webpage_SpaceAPI), currentSeconds);

        if (currentSeconds % interval_in_Seconds_Json == 0){    
                    Serial.println("MAIN: Returned Sensor Data:");
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


}
