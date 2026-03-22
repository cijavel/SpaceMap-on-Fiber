#include "NeoPixelLED.h"
#include <NeoPixelBus.h>
#include "AppConfig.h"

NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(LED_COUNT, LED_DATA_PIN);

// --------------------------------------------------------------------------
// Base color definitions for each hackerspace status.
// Brightness is applied at runtime via scaleBrightness().
// --------------------------------------------------------------------------
RgbColor colorOpen   (0,   255,   0);   // green  – space is open
RgbColor colorClosed (255,   0,   0);   // red    – space is closed
RgbColor colorUnknown(24,   12, 128);   // blue   – status unavailable
RgbColor colorWhite  (255, 255, 255);
RgbColor colorOff    (0);               // LED off

// --------------------------------------------------------------------------
// Init LEDs
// --------------------------------------------------------------------------
void NeoPixelLED::initLEDs() {
    strip.Begin();
    strip.ClearTo(colorOff);
    strip.Show();
}

// --------------------------------------------------------------------------
// Update hackerspace status on LED strip.
// Skips the update entirely if any LED index is out of range.
// --------------------------------------------------------------------------
void NeoPixelLED::updateLEDs(std::vector<SpaceStatusList>& spaceStatusList) {
    if (!validateLEDIndices(spaceStatusList)) {
        return;
    }

    uint8_t brightness = AppConfig::getInstance().getLedBrightness();

    for (const auto& entry : spaceStatusList) {
        RgbColor color;
        switch (entry.getStatus()) {
            case SpaceStatus::OPEN:    color = scaleBrightness(colorOpen,    brightness); break;
            case SpaceStatus::CLOSED:  color = scaleBrightness(colorClosed,  brightness); break;
            case SpaceStatus::UNKNOWN: color = scaleBrightness(colorUnknown, brightness); break;
            default:                   color = scaleBrightness(colorOff,     brightness); break;
        }
        strip.SetPixelColor(entry.getLED(), color);
    }
    strip.Show();
}

// --------------------------------------------------------------------------
// Blink a single LED white 10x (150ms on/off), then restore strip status.
// --------------------------------------------------------------------------
void NeoPixelLED::blinkLED(uint8_t ledIndex, std::vector<SpaceStatusList>& spaceStatusList) {
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

    updateLEDs(spaceStatusList);
}

// --------------------------------------------------------------------------
// LED startup sequence: cycle each LED through open/closed/unknown colors.
// delayMs is the total time (ms) spent on each LED position.
// --------------------------------------------------------------------------
void NeoPixelLED::enumerateLEDs(int delayMs) {
    uint8_t brightness = AppConfig::getInstance().getLedBrightness();

    for (int i = 0; i < LED_COUNT; i++) {
        strip.ClearTo(colorOff);
        strip.Show();
        delay(delayMs / 4);

        strip.SetPixelColor(i, scaleBrightness(colorOpen,    brightness)); strip.Show(); delay(delayMs / 4);
        strip.SetPixelColor(i, scaleBrightness(colorClosed,  brightness)); strip.Show(); delay(delayMs / 4);
        strip.SetPixelColor(i, scaleBrightness(colorUnknown, brightness)); strip.Show(); delay(delayMs / 4);
    }
    strip.ClearTo(colorOff);
    strip.Show();
}

// --------------------------------------------------------------------------
// Scale each RGB channel proportionally to the target brightness (0–255).
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
// Returns false and logs an error if any index is >= LED_COUNT.
// --------------------------------------------------------------------------
bool NeoPixelLED::validateLEDIndices(const std::vector<SpaceStatusList>& spaceStatusList) {
    for (const auto& entry : spaceStatusList) {
        if (entry.getLED() >= LED_COUNT) {
            Serial.println("------------------------------");
            Serial.println("ERROR: LED index out of range!");
            Serial.print("Space: ");
            Serial.println(entry.getName());
            Serial.print("LED index: ");
            Serial.print(entry.getLED());
            Serial.print(" / LED_COUNT: ");
            Serial.println(LED_COUNT);
            Serial.println("------------------------------");
            return false;
        }
    }
    return true;
}
