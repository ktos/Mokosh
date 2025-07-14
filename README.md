# Mokosh

![CI Status](https://github.com/ktos/Mokosh/actions/workflows/ci.yml/badge.svg)

Mokosh is a pseudo-framework with a goal to be a simple set of a functions for
easily built similarly operated IoT devices.

Mokosh is built on single non-static global object and wraps a few typical
things (connecting to Wi-Fi, connecting to MQTT, handling commands, recurring
operations) making them easier to use.

I have created Mokosh originally for a failed IoT start-up, then extracted code,
heavily refactored and started using in my own home IoT devices. Years later in
most cases I've switched to ESPHome, yet I am still using Mokosh every time I
need a custom device firmware.

## Usage

Code is prepared for the Arduino platform, building is tested using platformio.

Originally was prepared for ESP8266 (NodeMCU, WeMos D1 R2, ESP-01S), then ESP32
(tested on ESP32, ESP32-S3, ESP32-C3, ESP32-C6, ESP32-H2 on various boards), and
NRF52 (tested on Adafruit's Feather boards). Works both with Arduino Core 2.x
and Core 3.x for ESP32.

Of course, NRF52 or ESP32-H2 do not support Wi-Fi features.

You have to just include the main file somewhere in the beginning of your
Arduino sketch:

```cpp
#include <Mokosh.hpp>
```

Then you can just start using Mokosh object.

```cpp
Mokosh m("Mokosh", "1.0.0", false, true);
```

By default, Mokosh object needs:

* prefix of a device (e.g. Mokosh), the default name of the device will be
  Prefix_ID where ID is taken from ESP8266/ESP32 MAC or can be provided in build
  flags,
* version of the software, published to the MQTT server with "hello" message,
* should the filesystem be used, or is the config only in memory,
* should the Serial connection be used, and if so - serial logger is registered.

In your `loop()` function, there is a need to run Mokosh's loop, which will do everything needed:

```cpp
void loop()
{
    m.loop();
}
```

### Logging

The main feature is unified logging using `mlogV()`, `mlogI()`, `mlogW()` and
`mlogE()` family of macros. Usage of these is handled by various loggers:
Serial, Telnet or others you can prepare (e.g. OLED or WebSocket).

### Interval Functions

Using TickTwo library, Mokosh makes easy to register functions that will run on
defined interval, e.g.:

```cpp
m.registerIntervalFunction([&]()
{
    digitalWrite(LED, state ? HIGH : LOW);
    state = !state;
    mlogV("toggled LED");
},
500);
```

### Wi-Fi and MQTT connection

By default, Mokosh is connecting to defined Wi-Fi network (or multiple) and
connects to the provided MQTT broker, where IP address and version are
published, and then it stays waiting for commands on a special topic. However,
this can be overridden, by providing your own NETWORK or MQTT services, e.g.
using LoRa.

```cpp
#include <Mokosh.hpp>

// global object for the framework, disable using config file
Mokosh mokosh("Mokosh", "1.0.0", false);

void setup()
{
    srand(millis());

    // static configuration to be used: Wifi with yourssid and yourpassword password
    // mqtt broker at 192.168.1.10, port 1883
    mokosh.config->set(mokosh.config->key_ssid, "yourssid");
    mokosh.config->set(mokosh.config->key_wifi_password, "yourpass");
    mokosh.config->set(mokosh.config->key_broker, "192.168.1.10");
    mokosh.config->set(mokosh.config->key_broker_port, 1883);

    // device id will be Mokosh_ABCDE where ABCDE is a chip id
    mokosh.begin();

    // publish new random int message every 2000 ms
    mokosh.registerIntervalFunction(
        [&]()
        {
            auto mqtt = mokosh.getMqttService();
            mqtt->publish("rand", rand());
        },
        2000);
}

long lastMessage = 0;

void loop()
{
    // handle loop
    mokosh.loop();
}
```

It automatically allows sending also heartbeats to detect if the device is
"alive", the logs will look like that for this example:

```
(D t:2151ms) (begin lib/Mokosh/src/Mokosh.cpp:145) autoconnect, registering Wi-Fi as a network provider
..... ok
(I t:3476ms) (reconnect lib/Mokosh/src/MokoshWiFiService.hpp:178) IP: 192.168.8.182
(D t:3476ms) (begin lib/Mokosh/src/Mokosh.cpp:160) autoconnect, registering default MQTT provider
(I t:3476ms) (setup lib/Mokosh/src/PubSubClientService.hpp:36) MQTT broker set to 192.168.1.10 port 1883
(I t:3487ms) (reconnect lib/Mokosh/src/PubSubClientService.hpp:93) MQTT reconnected
(I t:3488ms) (hello lib/Mokosh/src/Mokosh.cpp:109) Sending hello
(D t:3489ms) (publishRaw lib/Mokosh/src/PubSubClientService.hpp:146) Publishing message on topic Mokosh_FFFE4D/version: 1.0.0
(D t:3491ms) (publishShortVersion lib/Mokosh/src/Mokosh.cpp:372) Version: 1.0.0
(V t:3491ms) (publishIP lib/Mokosh/src/Mokosh.cpp:210) Sending IP: 192.168.8.182
(D t:3492ms) (publishRaw lib/Mokosh/src/PubSubClientService.hpp:146) Publishing message on topic Mokosh_FFFE4D/debug/ip: {"ipaddress": "192.168.8.182"}
(D t:3493ms) (registerIntervalFunction lib/Mokosh/src/Mokosh.cpp:479) Registering interval function on time 10000
(I t:3493ms) (begin lib/Mokosh/src/Mokosh.cpp:185) Starting operations...
(D t:3493ms) (registerIntervalFunction lib/Mokosh/src/Mokosh.cpp:479) Registering interval function on time 2000
(D t:3493ms) (registerIntervalFunction lib/Mokosh/src/Mokosh.cpp:485) Called after begin(), running ticker immediately
(D t:5493ms) (publishRaw lib/Mokosh/src/PubSubClientService.hpp:146) Publishing message on topic Mokosh_FFFE4D/rand: 412788896.00
(D t:7493ms) (publishRaw lib/Mokosh/src/PubSubClientService.hpp:146) Publishing message on topic Mokosh_FFFE4D/rand: 1401777536.00
(D t:9493ms) (publishRaw lib/Mokosh/src/PubSubClientService.hpp:146) Publishing message on topic Mokosh_FFFE4D/rand: 1388557056.00
(D t:11493ms) (publishRaw lib/Mokosh/src/PubSubClientService.hpp:146) Publishing message on topic Mokosh_FFFE4D/rand: 1369425024.00
(D t:13493ms) (publishRaw lib/Mokosh/src/PubSubClientService.hpp:146) Publishing message on topic Mokosh_FFFE4D/debug/heartbeat: 13493.00
```

### Configuration

Mokosh provides access to the configuration file or set of a configuration
parameters in memory, used for storing Wi-Fi connection information, but can be
extended to store any settings needed. File storage is realized using LittleFS.

## Dependencies

The framework is dependent on the following libraries:

* [ArduinoJson](https://github.com/bblanchon/ArduinoJson), ~6.21.0,
* [TickTwo](https://github.com/sstaub/TickTwo), ~4.4.0,
* [PubSubClient3](https://github.com/hmueller01/pubsubclient3), ~3.1.0.
