#include <Mokosh.h>

void setup() {
    // use built-in LED, not NeoPixel strip
    Error_configure(true, false);
}

void loop() {
    // will "blink out" the ERROR_CONFIG 
    Error_show(ERROR_CONFIG);

    delay(1000);
}