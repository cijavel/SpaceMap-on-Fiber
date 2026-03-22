#ifndef SPACE_API_ON_FIBER_NEOPIXEL_H
#define SPACE_API_ON_FIBER_NEOPIXEL_H

#include "Configuration.h"
#include <vector>
#include "DataStructure.h"
#include "DataSpaceList.h"
#include <NeoPixelBus.h>
#include <freertos/semphr.h>

// Wraps the NeoPixelBus LED strip. All operations go through this singleton.
//
// Thread-safety: _stripMutex serialises all strip access so that the
// Arduino loop task and any FreeRTOS task (e.g. blink_task) never touch
// the NeoPixelBus strip object at the same time.
class NeoPixelLED {
public:
    static NeoPixelLED& getInstance() {
        static NeoPixelLED instance;
        return instance;
    }

    // Clear the strip, prepare it for use, and create the strip mutex.
    // Must be called once in setup() before any other method.
    void initLEDs();

    // Run a startup sequence that cycles each LED through all status colors.
    // delayMs controls the total time spent per LED in milliseconds.
    void enumerateLEDs(int delayMs);

    // Update the strip to reflect the current open/closed status of each tracked space.
    void updateLEDs(std::vector<SpaceStatusList>& spaceStatusList);

    // Blink a single LED white 10x (150ms on/off), then restore the full strip status.
    // Safe to call from any FreeRTOS task; acquires _stripMutex for the full duration
    // so it never races with updateLEDs().
    void blinkLED(uint8_t ledIndex, std::vector<SpaceStatusList>& spaceStatusList);

private:
    NeoPixelLED() : _stripMutex(nullptr) {}
    NeoPixelLED(NeoPixelLED const&) = delete;
    void operator=(NeoPixelLED const&) = delete;

    // Mutex that must be held for every read/write access to the strip object.
    // Created once in initLEDs(). Never deleted (singleton lifetime).
    SemaphoreHandle_t _stripMutex;

    // ----- Internal helpers (called with _stripMutex already held) -----

    // Apply status colours to the strip without acquiring the mutex.
    // Returns false if any LED index is out of range (strip is left unchanged).
    bool updateLEDsUnsafe(std::vector<SpaceStatusList>& spaceStatusList);

    // Returns false (and prints a warning) if any LED index in the list is >= LED_COUNT.
    bool validateLEDIndices(const std::vector<SpaceStatusList>& spaceStatusList);

    // Scales a color to the given brightness level (0-255).
    RgbColor scaleBrightness(RgbColor color, int brightness);
};

#endif // SPACE_API_ON_FIBER_NEOPIXEL_H
