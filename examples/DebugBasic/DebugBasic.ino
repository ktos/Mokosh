#include <Mokosh.hpp>

Mokosh mokosh;

void customCommand(uint8_t* message, unsigned int length) {
    mokosh.publish("test", 1.0f);
}

void setup() {
    MokoshConfiguration mc = create_configuration("kilibar_iot", "RASengan91", "192.168.8.12", 1883, "", 80, "");
    
    mokosh.setConfiguration(mc);
    mokosh.setDebugLevel(DebugLevel::VERBOSE);
    mokosh.onCommand(customCommand);
    mokosh.begin("Mokosh");
}

void loop() {
    mokosh.loop();
}