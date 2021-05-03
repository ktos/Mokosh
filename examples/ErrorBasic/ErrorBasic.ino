#include <Mokosh.hpp>

Mokosh m;

void setup() {
    // will automatically reboot if error is met
    m.setRebootOnError(true);
    m.begin("Mokosh");    
}

void loop() {
    delay(1000);
    m.error(201); // throw error code 201 (will reboot)
}