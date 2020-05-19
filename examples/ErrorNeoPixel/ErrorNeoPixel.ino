#define ENABLE_NEOPIXEL
#include <FastLED.h>
#include <Mokosh.h>

void setup() {
    // use built-in LED, not NeoPixel strip
    Debug_init(DLVL_DEBUG);
    Error_configure(false, true);
    NeoPixel_setup(1);
}

void loop() {
    // will show ERROR_DISCOVERY using NeoPixel strip
    Error_show(ERROR_DISCOVERY);
}