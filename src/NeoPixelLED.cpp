
#include "NeoPixelLED.h"
#include <NeoPixelBus.h>

// three element pixels, in different order and speeds
NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(LED_COUNT, LED_DATA_PIN);

// --------------------------------------------------------------------------
// Definition of color
// --------------------------------------------------------------------------
RgbColor copen(0,255,0);
RgbColor cclosed(255,0,0);
RgbColor cunknown(24,12,128);
RgbColor cwhite( 255,255,255);
RgbColor cblack(0);


// --------------------------------------------------------------------------
// init Leds
// --------------------------------------------------------------------------
void NeoPixelLED::initLEDs() {
    strip.Begin();
    strip.ClearTo(cblack);
    strip.Show();

    #ifdef RGB_BUILTIN
        digitalWrite(RGB_BUILTIN, LOW);    // Turn the RGB LED off. Turn onboard LED off. HIGH to turn on
        //neopixelWrite(RGB_BUILTIN,5,0,0); // Red
        //delay(1000);
    #endif
    
 }

// --------------------------------------------------------------------------
// update Hackerspace status on led strip
// --------------------------------------------------------------------------
void NeoPixelLED::updateLEDs(std::vector<SpaceStatusList> &spacestatus, unsigned long currentSeconds){


    
    if (currentSeconds % interval_in_Seconds_LEDs == 0){  

        #ifdef RGB_BUILTIN
            digitalWrite(RGB_BUILTIN, LOW);    // Turn the RGB LED off. Turn onboard LED off. HIGH to turn on
            //neopixelWrite(RGB_BUILTIN,5,0,0); // Red
            //delay(1000);
        #endif

        if (checknumberofLEDs(spacestatus) == true){ 
            for (const auto& item : spacestatus) {

                if (item.getStatus() == SpaceStatus::OPEN) {
                    // red and green are swapped for our PL9823 leds :)

                    strip.SetPixelColor(item.getLED(), setBrightness(copen, LED_BRIGHTNESS));
                } else if(item.getStatus() == SpaceStatus::CLOSED) {
                    strip.SetPixelColor(item.getLED(), setBrightness(cclosed, LED_BRIGHTNESS));
                } else if(item.getStatus() == SpaceStatus::UNKNOWN) {
                    strip.SetPixelColor(item.getLED(), setBrightness(cunknown, LED_BRIGHTNESS));
                } else {
                    strip.SetPixelColor(item.getLED(), setBrightness(cblack, LED_BRIGHTNESS));
                }
            }
            strip.Show();
        }
    }

}

// --------------------------------------------------------------------------
// led test - initial phase
// --------------------------------------------------------------------------
void NeoPixelLED::enumerateLEDs( int delay_time) {
    int i ;

    for (i = 0; i < LED_COUNT; i++){
        strip.ClearTo(cblack);
        strip.Show();
        delay(delay_time/4);
        strip.SetPixelColor(i, setBrightness(copen, LED_BRIGHTNESS));
        strip.Show();
        delay(delay_time/4);
        strip.SetPixelColor(i, setBrightness(cclosed, LED_BRIGHTNESS));
        strip.Show();
        delay(delay_time/4);
        strip.SetPixelColor(i, setBrightness(cunknown, LED_BRIGHTNESS));
        strip.Show();
        delay(delay_time/4);
    }
    strip.ClearTo(cblack);
}

// --------------------------------------------------------------------------
// set the Brightness of led
// --------------------------------------------------------------------------
RgbColor NeoPixelLED::setBrightness(RgbColor color, int brightness) {
  color = RgbColor(
    (color.R * brightness) / 255,
    (color.G * brightness) / 255,
    (color.B * brightness) / 255
  );
  return color;
}


// --------------------------------------------------------------------------
// set the Brightness of led Random
// --------------------------------------------------------------------------
RgbColor NeoPixelLED::setBrightnessStar(RgbColor color, int brightness, int variante) {

    if (brightness <= variante){
        variante = 0;
    }

    int randomValue = random(1, (brightness-variante)+1); 
  


  color = RgbColor(
    (color.R * randomValue ) / 255,
    (color.G * randomValue) / 255,
    (color.B * randomValue) / 255
  );
  return color;
}

// --------------------------------------------------------------------------
// set the Brightness of led Colorshift
// --------------------------------------------------------------------------
RgbColor NeoPixelLED::setBrightnessStarColorshift(RgbColor color, int brightness, int colorshift) {
  int randomValue = random(-1 * colorshift, colorshift); 

  color = RgbColor(
    (color.R * (brightness + randomValue)) / 255,
    (color.G * (brightness + randomValue)) / 255,
    (color.B * (brightness + randomValue)) / 255
  );
  return color;
}





// --------------------------------------------------------------------------
// heck, if we got enough leds for all Hackerspace in list
// --------------------------------------------------------------------------
bool NeoPixelLED::checknumberofLEDs(std::vector<SpaceStatusList> &spacestatus) {
    bool b = true;
    if (spacestatus.size() > LED_COUNT) {
        b = false;
        Serial.println("------------------------------");
        Serial.println("Please add more LEDs !!!!!!!!!");
        Serial.print("Spaces: ");
        Serial.println(spacestatus.size());
        Serial.print("LEDs: ");
        Serial.println(LED_COUNT);
        Serial.println("------------------------------");
    }
    return b;
 }
