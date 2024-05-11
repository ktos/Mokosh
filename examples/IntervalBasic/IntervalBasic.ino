#include <Mokosh.hpp>

Mokosh mokosh("Mokosh");

void some_func()
{
    auto mqtt = mokosh.getMqttService();
    mqtt->publish("test", "alive!");
}

void setup()
{
    mokosh.begin();

    mokosh.registerIntervalFunction(some_func, 1200); // register some_func to run every 1.2 seconds
}

void loop()
{
    mokosh.loop(); // must be to interval and MQTT to work
}