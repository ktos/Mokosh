#include <Mokosh.hpp>

// global object for the framework
Mokosh mokosh;

void setup() {
    srand(millis());

    // static configuration to be used: Wifi with yourssid and yourpassword password
    // mqtt broker at 192.168.1.10, port 1883

    mokosh.setConfigFile(false);
    mokosh.config.set(mokosh.config.key_ssid, "yourssid");
    mokosh.config.set(mokosh.config.key_wifi_password, "yourpassword");
    mokosh.config.set(mokosh.config.key_broker, "192.168.1.10");
    mokosh.config.set(mokosh.config.key_broker_port, 1883);

    // device id will be Mokosh_ABCDE where ABCDE is a chip id
    mokosh.begin("Mokosh");
}

long lastMessage = 0;

void loop() {
    // publish new random int message every 2000 ms
    
    if (millis() - lastMessage > 2000) {        
        // will publish to Mokosh_ABCDE/rand topic
        mokosh.publish("rand", rand());
        lastMessage = millis();
    }

    // handle loop
    mokosh.loop();
}