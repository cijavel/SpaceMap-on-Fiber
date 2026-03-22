#ifndef SPACE_API_ON_FIBER_NEOPIXEL_H
#define SPACE_API_ON_FIBER_NEOPIXEL_H

#include "Configuration.h"
#include <vector>
#include "DataStructure.h"
#include "DataSpaceList.h"
#include <NeoPixelBus.h>

// Wraps the NeoPixelBus LED strip. All operations go through this singleton.
class NeoPixelLED {
public:
    static NeoPixelLED& getInstance() {
        static NeoPixelLED instance;
        return instance;
    }

    // Clear the strip and prepare it for use. Call once in setup().
    void initLEDs();

    // Run a startup sequence that cycles each LED through all status colors.
    // delayMs controls the total time spent per LED in milliseconds.
    void enumerateLEDs(int delayMs);

    // Update the strip to reflect the current open/closed status of each tracked space.
    void updateLEDs(std::vector<SpaceStatusList>& spaceStatusList);

    // Blink a single LED white 10x (150ms on/off), then restore the full strip status.
    void blinkLED(uint8_t ledIndex, std::vector<SpaceStatusList>& spaceStatusList);

private:
    NeoPixelLED() {}
    NeoPixelLED(NeoPixelLED const&) = delete;
    void operator=(NeoPixelLED const&) = delete;

    // Returns false (and prints a warning) if any LED index in the list is >= LED_COUNT.
    bool validateLEDIndices(const std::vector<SpaceStatusList>& spaceStatusList);

    // Scales a color to the given brightness level (0–255).
    RgbColor scaleBrightness(RgbColor color, int brightness);
};

#endif // SPACE_API_ON_FIBER_NEOPIXEL_H
