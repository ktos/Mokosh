# Mokosh

[![Build Status](https://dev.azure.com/ktos/Mokosh/_apis/build/status/Mokosh%20Tag?branchName=master)](https://dev.azure.com/ktos/Mokosh/_build/latest?definitionId=12&branchName=master)

Mokosh is a pseudo-framework, extracted from the "Chione" project, with a goal
to be a simple set of a functions for easily built similarly operated IoT
devices.

Mokosh is built on single non-static global object and wraps a few typical
things (connecting to Wi-Fi, connecting to MQTT, handling some basic commands)
making them easier to use.

This version of Mokosh is heavily in development.

## Usage

Code is prepared for the Arduino platform, building is tested on arduino-cli,
currently 0.13. Prepared for ESP8266 and ESP32, tested on NodeMCU, WeMos D1 R2,
ESP-01S and ESP32-WROOM-32D.

You have to just include the main file somewhere in the beginning of your
Arduino sketch:

```cpp
#include <Mokosh.hpp>
```

Then you can just start using Mokosh object.

### Dependencies
The framework is dependent on the following libraries:

* [ArduinoJson](https://github.com/bblanchon/ArduinoJson), 6.17.2,
* [RemoteDebug](https://github.com/JoaoLopesF/RemoteDebug), 3.0.5,
* [PubSubClient](https://github.com/knolleary/pubsubclient), 2.7.0.
