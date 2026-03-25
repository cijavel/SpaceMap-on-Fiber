#include "NeoPixelLED.h"
#include <NeoPixelBus.h>
#include "AppConfig.h"
#include "DataSpaceList.h"

NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(LED_SLOT_COUNT, LED_DATA_PIN);

// --------------------------------------------------------------------------
// Base color definitions for each hackerspace status.
// Brightness is applied at runtime via scaleBrightness().
// --------------------------------------------------------------------------
RgbColor colorOpen   (0,   255,   0);   // green  - space is open
RgbColor colorClosed (255,   0,   0);   // red    - space is closed
RgbColor colorUnknown(24,   12, 128);   // blue   - status unavailable
RgbColor colorWhite  (255, 255, 255);
RgbColor colorOff    (0);               // LED off

// --------------------------------------------------------------------------
// Init LEDs
// Creates the strip mutex. Must be called once in setup() before anything
// else touches the strip.
// --------------------------------------------------------------------------
void NeoPixelLED::initLEDs() {
    _stripMutex = xSemaphoreCreateMutex();

    strip.Begin();
    strip.ClearTo(colorOff);
    strip.Show();
}

// --------------------------------------------------------------------------
// Update hackerspace status on LED strip (public, mutex-protected).
// Skips the update entirely if any LED index is out of range.
// --------------------------------------------------------------------------
void NeoPixelLED::updateLEDs(std::vector<SpaceStatusList>& spaceStatusList) {
    if (xSemaphoreTake(_stripMutex, portMAX_DELAY) != pdTRUE) return;
    updateLEDsUnsafe(spaceStatusList);
    xSemaphoreGive(_stripMutex);
}

// --------------------------------------------------------------------------
// Internal: apply status colours without acquiring the mutex.
// Caller must already hold _stripMutex.
// --------------------------------------------------------------------------
bool NeoPixelLED::updateLEDsUnsafe(std::vector<SpaceStatusList>& spaceStatusList) {
    if (!validateLEDIndices(spaceStatusList)) {
        return false;
    }

    uint8_t brightness = AppConfig::getInstance().getLedBrightness();

    // Rebuild disabled lookup from the current mapping — O(n), no heap allocation.
    memset(_disabledByLed, 0, sizeof(_disabledByLed));
    const auto& mapping = DataSpaceList::getInstance().getList();
    for (const auto& m : mapping) {
        if (m.getLED() < LED_SLOT_COUNT) _disabledByLed[m.getLED()] = m.isDisabled();
    }

    for (const auto& entry : spaceStatusList) {
        RgbColor color;
        if (_disabledByLed[entry.getLED()]) {
            color = colorOff;
        } else {
            switch (entry.getStatus()) {
                case SpaceStatus::OPEN:    color = scaleBrightness(colorOpen,    brightness); break;
                case SpaceStatus::CLOSED:  color = scaleBrightness(colorClosed,  brightness); break;
                case SpaceStatus::UNKNOWN: color = scaleBrightness(colorUnknown, brightness); break;
                default:                   color = scaleBrightness(colorOff,     brightness); break;
            }
        }
        strip.SetPixelColor(entry.getLED(), color);
    }
    strip.Show();
    return true;
}

// --------------------------------------------------------------------------
// Blink a single LED white 10x (150ms on/off), then restore strip status.
//
// Acquires _stripMutex for the entire duration (including the final restore).
// This prevents any concurrent updateLEDs() call from interrupting the blink
// sequence, and ensures only one blink_task can run at a time (Bug A + C fix).
//
// Calls updateLEDsUnsafe() for the final restore to avoid a recursive lock
// on _stripMutex (which is a standard, non-recursive mutex).
// --------------------------------------------------------------------------
void NeoPixelLED::blinkLED(uint8_t ledIndex, std::vector<SpaceStatusList>& spaceStatusList) {
    if (xSemaphoreTake(_stripMutex, portMAX_DELAY) != pdTRUE) return;

    uint8_t brightness = AppConfig::getInstance().getLedBrightness();
    RgbColor white = scaleBrightness(colorWhite, brightness);

    for (int i = 0; i < 10; i++) {
        strip.SetPixelColor(ledIndex, white);
        strip.Show();
        delay(150);
        strip.SetPixelColor(ledIndex, colorOff);
        strip.Show();
        delay(150);
    }

    // Restore full strip state while still holding the mutex.
    updateLEDsUnsafe(spaceStatusList);

    xSemaphoreGive(_stripMutex);
}

// --------------------------------------------------------------------------
// LED startup sequence: cycle each LED through open/closed/unknown colors.
// delayMs is the total time (ms) spent on each LED position.
// Called from setup() before the FreeRTOS scheduler hands off to loop(),
// so no mutex needed here — but we take it anyway for correctness.
// --------------------------------------------------------------------------
void NeoPixelLED::enumerateLEDs(int totalMs) {
    if (xSemaphoreTake(_stripMutex, portMAX_DELAY) != pdTRUE) return;

    uint8_t brightness = AppConfig::getInstance().getLedBrightness();
    // Divide total duration evenly across all LEDs; minimum 1 ms per LED.
    int perLed = max(1, totalMs / LED_SLOT_COUNT);

    for (int i = 0; i < LED_SLOT_COUNT; i++) {
        strip.ClearTo(colorOff);
        strip.Show();
        delay(perLed / 4);

        strip.SetPixelColor(i, scaleBrightness(colorOpen,    brightness)); strip.Show(); delay(perLed / 4);
        strip.SetPixelColor(i, scaleBrightness(colorClosed,  brightness)); strip.Show(); delay(perLed / 4);
        strip.SetPixelColor(i, scaleBrightness(colorUnknown, brightness)); strip.Show(); delay(perLed / 4);
    }
    strip.ClearTo(colorOff);
    strip.Show();

    xSemaphoreGive(_stripMutex);
}

// --------------------------------------------------------------------------
// Scale each RGB channel proportionally to the target brightness (0-255).
// --------------------------------------------------------------------------
RgbColor NeoPixelLED::scaleBrightness(RgbColor color, int brightness) {
    return RgbColor(
        (color.R * brightness) / 255,
        (color.G * brightness) / 255,
        (color.B * brightness) / 255
    );
}

// --------------------------------------------------------------------------
// Check that all LED indices are within the physical strip bounds.
// Returns false and logs an error if any index is >= LED_SLOT_COUNT.
// --------------------------------------------------------------------------
bool NeoPixelLED::validateLEDIndices(const std::vector<SpaceStatusList>& spaceStatusList) {
    for (const auto& entry : spaceStatusList) {
        if (entry.getLED() >= LED_SLOT_COUNT) {
            Serial.println("------------------------------");
            Serial.println("ERROR: LED index out of range!");
            Serial.print("Space: ");
            Serial.println(entry.getName());
            Serial.print("LED index: ");
            Serial.print(entry.getLED());
            Serial.print(" / LED_SLOT_COUNT: ");
            Serial.println(LED_SLOT_COUNT);
            Serial.println("------------------------------");
            return false;
        }
    }
    return true;
}
