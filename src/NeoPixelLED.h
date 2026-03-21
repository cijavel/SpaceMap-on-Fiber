#ifndef SPACE_API_ON_FIBER_NEOPIXEL_H
#define SPACE_API_ON_FIBER_NEOPIXEL_H

#include "Configuration.h"
#include <vector>
#include "DataStructure.h"
#include "DataSpaceList.h"
#include <NeoPixelBus.h>

class NeoPixelLED {
public:
    static NeoPixelLED &getInstance() {
        static NeoPixelLED instance;
        return instance;
    }

    void updateLEDs(std::vector<SpaceStatusList> &spacestatus);
    void enumerateLEDs(int delay_time);
    void initLEDs();

private:
    NeoPixelLED() {}
    NeoPixelLED(NeoPixelLED const&) = delete;
    void operator=(NeoPixelLED const&) = delete;

    // Returns false if any LED index in the list is >= LED_COUNT.
    bool validateLEDIndices(const std::vector<SpaceStatusList> &spacestatus);
    RgbColor setBrightness(RgbColor color, int brightness);
};

#endif // SPACE_API_ON_FIBER_NEOPIXEL_H
