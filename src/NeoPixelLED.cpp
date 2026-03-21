#include "NeoPixelLED.h"
#include <NeoPixelBus.h>

NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(LED_COUNT, LED_DATA_PIN);

// --------------------------------------------------------------------------
// Color definitions
// --------------------------------------------------------------------------
RgbColor copen(0, 255, 0);
RgbColor cclosed(255, 0, 0);
RgbColor cunknown(24, 12, 128);
RgbColor cwhite(255, 255, 255);
RgbColor cblack(0);

// --------------------------------------------------------------------------
// Init LEDs
// --------------------------------------------------------------------------
void NeoPixelLED::initLEDs() {
    strip.Begin();
    strip.ClearTo(cblack);
    strip.Show();
}

// --------------------------------------------------------------------------
// Update Hackerspace status on LED strip
// --------------------------------------------------------------------------
void NeoPixelLED::updateLEDs(std::vector<SpaceStatusList> &spacestatus) {
    if (!validateLEDIndices(spacestatus)) {
        return;
    }
    for (const auto& item : spacestatus) {
        RgbColor color;
        switch (item.getStatus()) {
            case SpaceStatus::OPEN:    color = setBrightness(copen,    LED_BRIGHTNESS); break;
            case SpaceStatus::CLOSED:  color = setBrightness(cclosed,  LED_BRIGHTNESS); break;
            case SpaceStatus::UNKNOWN: color = setBrightness(cunknown, LED_BRIGHTNESS); break;
            default:                   color = setBrightness(cblack,   LED_BRIGHTNESS); break;
        }
        strip.SetPixelColor(item.getLED(), color);
    }
    strip.Show();
}

// --------------------------------------------------------------------------
// LED test – startup sequence
// --------------------------------------------------------------------------
void NeoPixelLED::enumerateLEDs(int delay_time) {
    for (int i = 0; i < LED_COUNT; i++) {
        strip.ClearTo(cblack);
        strip.Show();
        delay(delay_time / 4);

        strip.SetPixelColor(i, setBrightness(copen,    LED_BRIGHTNESS)); strip.Show(); delay(delay_time / 4);
        strip.SetPixelColor(i, setBrightness(cclosed,  LED_BRIGHTNESS)); strip.Show(); delay(delay_time / 4);
        strip.SetPixelColor(i, setBrightness(cunknown, LED_BRIGHTNESS)); strip.Show(); delay(delay_time / 4);
    }
    strip.ClearTo(cblack);
    strip.Show();
}

// --------------------------------------------------------------------------
// Scale a color to a given brightness (0–255)
// --------------------------------------------------------------------------
RgbColor NeoPixelLED::setBrightness(RgbColor color, int brightness) {
    return RgbColor(
        (color.R * brightness) / 255,
        (color.G * brightness) / 255,
        (color.B * brightness) / 255
    );
}

// --------------------------------------------------------------------------
// Validate that all LED indices in the status list are within strip bounds.
// Returns false and prints a warning if any index would overflow.
// --------------------------------------------------------------------------
bool NeoPixelLED::validateLEDIndices(const std::vector<SpaceStatusList> &spacestatus) {
    for (const auto& item : spacestatus) {
        if (item.getLED() >= LED_COUNT) {
            Serial.println("------------------------------");
            Serial.println("ERROR: LED index out of range!");
            Serial.print("Space: ");
            Serial.println(item.getName());
            Serial.print("LED index: ");
            Serial.print(item.getLED());
            Serial.print(" / LED_COUNT: ");
            Serial.println(LED_COUNT);
            Serial.println("------------------------------");
            return false;
        }
    }
    return true;
}
