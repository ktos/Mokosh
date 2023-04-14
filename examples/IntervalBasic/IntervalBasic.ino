#include <Mokosh.hpp>

Mokosh mokosh;

void some_func()
{
    mokosh.publish("test", "alive!");
}

void setup()
{
    mokosh.begin("Mokosh");

    mokosh.registerIntervalFunction(some_func, 1200); // register some_func to run every 1.2 seconds
}

void loop()
{
    mokosh.loop(); // must be to interval and MQTT to work
}