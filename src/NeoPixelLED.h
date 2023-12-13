#ifndef SPACE_API_ON_FIBER_NEOPIXEL_H
#define SPACE_API_ON_FIBER_NEOPIXEL_H


#include "Configuration.h"
#include <vector>
#include "DataStructure.h"
#include "DataSpaceList.h"
#include <NeoPixelBus.h>

class NeoPixelLED{
public:

    static NeoPixelLED &getInstance() {
            static NeoPixelLED instance; // Guaranteed to be destroyed.
            return instance;// Instantiated on first use.
    };
    void updateLEDs(std::vector<SpaceStatusList> &spacestatus, unsigned long currentSeconds);
    void enumerateLEDs( int delay_time);
    void initLEDs();
    bool checknumberofLEDs(std::vector<SpaceStatusList> &spacestatus);

private:
    NeoPixelLED() {};                   // Constructor? (the {} brackets) are needed here.
    NeoPixelLED(NeoPixelLED const&);   // Don't Implement
    int transformColor(int color);
    RgbColor setBrightness(RgbColor color, int brightness);

};
#endif //SPACE_API_ON_FIBER_NEOPIXEL_H
