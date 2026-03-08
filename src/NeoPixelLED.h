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
    void updateLEDs(std::vector<SpaceStatusList> &spacestatus);
    void enumerateLEDs( int delay_time);
    void initLEDs();

private:
    NeoPixelLED() {};
    NeoPixelLED(NeoPixelLED const&);
    bool checknumberofLEDs(std::vector<SpaceStatusList> &spacestatus);
    RgbColor setBrightness(RgbColor color, int brightness);
    RgbColor setBrightnessStar(RgbColor color, int brightness, int variante);
    RgbColor setBrightnessStarColorshift(RgbColor color, int brightness, int colorshift);

};
#endif //SPACE_API_ON_FIBER_NEOPIXEL_H
