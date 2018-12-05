#include <Mokosh.h>

void setup() {
    // will start AP with SSID "Mokosh_XXXXX", where XXXXX is a internal
    // chip ID, listening on port 80 at 192.168.4.1
    FirstRun_configure("1.0", "My IoT Device 2018", "2018-01-01");
    FirstRun_start("Mokosh");
}

void loop() {
    // handle a connection of a new client
    FirstRun_handle();
}