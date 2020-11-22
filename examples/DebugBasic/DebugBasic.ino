#include <Mokosh.hpp>

Mokosh mokosh;

void setup() {
    MokoshConfiguration mc = create_configuration("kilibar_iot", "RASengan91", "192.168.8.12", 1883, "", 80, "");

    mokosh.disableFS();
    mokosh.setConfiguration(mc);
    mokosh.setDebugLevel(DebugLevel::VERBOSE);
    mokosh.begin("Mokosh");
}

void loop() {
    mokosh.loop();
}