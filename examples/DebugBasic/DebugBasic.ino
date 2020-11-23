#include <Mokosh.hpp>

Mokosh mokosh;

void customCommand(uint8_t* message, unsigned int length) {
    mokosh.publish("test", 1.0f);
    mdebugE("Error!");
}

void sendData() {
    mokosh.publish("rand", rand());
    mdebugI("Publishing random values to MQTT");
}

void setup() {
    //MokoshConfiguration mc = Mokosh::CreateConfiguration("kilibar_iot", "RASengan91", "192.168.8.12", 1883);
    //mokosh.setConfiguration(mc);

    mokosh.setDebugLevel(DebugLevel::VERBOSE);
    mokosh.onCommand(customCommand);

    mokosh.begin("Mokosh");
    //mokosh.onInterval(sendData, 2000);
}

void loop() {
    mokosh.loop();
}