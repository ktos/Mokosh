# Mokosh

[![Build Status](https://dev.azure.com/ktos/Mokosh/_apis/build/status/Mokosh%20Tag?branchName=master)](https://dev.azure.com/ktos/Mokosh/_build/latest?definitionId=12&branchName=master)

Mokosh is a pseudo-framework, extracted from the "Chione" project, with a goal
to be a simple set of a functions for easily built similarly operated IoT
devices.

Mokosh is built on the set of functions, not objects, and is mostly C-based,
instead of C++, sometimes relying on the global variables.

This version of Mokosh is heavily in development.

## Usage

Code is prepared for the Arduino platform, building is tested on arduino-cli,
currently 0.10. Prepared only for ESP8266, tested on NodeMCU and WeMos D1 R2.

You have to just include the main file somewhere in the beginning of your
Arduino sketch:

```cpp
#include <Mokosh.h>
```

Then you can just start using Mokosh functions.

### Dependencies
The library is dependent on the following libraries:

* [FastLED](https://github.com/FastLED/FastLED), 3.3.2,
* [ArduinoJson](https://github.com/bblanchon/ArduinoJson), 5.13.4,
* [PubSubClient](https://github.com/knolleary/pubsubclient), 2.7.0.

## Structure

### Debug.h
Contains functions for sending debug messages to the serial port and debug
levels definitions.

Serial output (by default 115200 kbps) verbosity is defined by minlevel
value in setting up debug, allowing values 1, 2, 4 or 8.

When the level is set to a certain value, all messages with level defined as
higher or equal are sent to a serial port. E.g.: `minlevel` in init set to 2
will allow sending messages with level set to `DLVL_INFO` (2), `DLVL_WARNING`
(4) and `DLVL_ERROR` (8), but not `DLVL_DEBUG` (1).

#### Available functions
`void Debug_init(uint8_t minlevel)`
Sets up the Debug system with a minimum level of messages to be passed through
to the serial output.

`uint8_t Debug_currentLevel()`
Gets the current debug level.

`void Debug_print(uint8_t level, const char* topic, {message});`
Prints debug information of the defined level and defined topic (so you can
group messages about similar things). Debug message may be on of the following
types:

* String,
* float,
* int,
* uint32_t.

### Error.h
Will "blink out" errors or show error codes using the NeoPixel strip. It is
based on the numerical value of errors, defined are:

* `ERROR_CONFIG`, value 10, error reading or parsing configuration file,
* `ERROR_SPIFFS`, value 50, SPIFFS error,
* `ERROR_WIFI`, value 100, Wi-Fi connection error
* `ERROR_DISCOVERY`, value 500, currently deprecated,
* `ERROR_SENSOR`, value 1000, currently not used,
* `ERROR_BROKER`, value 2000, MQTT connection error,
* `ERROR_CUSTOM`, value 10000, other error.

Codes are "blinked" using built-in NodeMCU LED - the LED is turned off and on
for the time defined in numerical error value, treated as number of milliseconds,
thus for ERROR_CONFIG there is 10 times blinks for 10 milliseconds every, 10
seconds delay and again.

Showing error using NeoPixel strip works that there are two red LEDs lighten up
and one is not, and then:

* one green for ERROR_CONFIG,
* one blue for ERROR_SPIFFS,
* one red for ERROR_WIFI,
* four red for ERROR_SENSOR,
* five red for ERROR_BROKER,
* yellow and four red for ERROR_CUSTOM.

After 60 seconds (for NeoPixel) or for 30 "blinking out" errors the device will
restart.

#### Available functions
`void Error_configure(bool useBuiltInLed, bool useNeoPixel)`
Sets up the Error system allowing you to choose between using built-in LED
or NeoPixel strip.

`void Error_show(uint16_t errorCode)`
Shows (using LED or NeoPixel strip) the error of the specified code.

### FirstRun.h
Is responsible for the "first run configuration" dialog. In the first run mode
the device will create an Access Point named PREFIX_XXXXX, where PREFIX is
defined by you and XXXXX is the internal chip ID.

Then it will start HTTP server at 192.168.4.1 on port 80.

Possible addresses to visit on the server:

* `/` -> shows welcome message,
* `/config` (POST) -> will receive `config.json` file and save it to
SPIFFS,
* `/reboot` (POST) -> will restart the device,
* `/whois` -> will return the JSON file with device information: device id,
software version and informational version where you can put additional
information, date of build (for builds with DEBUG_LEVEL set to 1) and MD5
checksum of the current firmware.

#### Available functions
`void FirstRun_configure(const char* _version, const char* _informationalVersion, const char* _buildDate)`
Sets up the First Run system. Requires passing current version info string,
and informational version (e.g. build number) and build date for presenting
this information in response to `/whois`.

`void FirstRun_start(char* prefix)`
Starts the Wi-Fi AP with a name of `prefix` + `_XXXXX`, starts the HTTP server
and starts listening for a client.

`void FirstRun_handle();`
Handles connection with a new client.

### MQTT.h
Will communicate with a MQTT broker. By default the device will subscribe
the topic `host/cmd`, awaiting for commands. You have to provide
your own function responding to the commands using Mqtt_setCallback().

#### Available functions
`bool Mqtt_isConnected()`
Returns if there is a connection with MQTT broker.

`bool Mqtt_loop()`
Handles MQTT subscription - receiving messages and commands.

`void Mqtt_setup(const char* broker, uint16_t port)`
Sets up the connection to a specific broker on a specific port.

`bool Mqtt_reconnect()`
Reconnects or connects to the broker and subscribe to the command topic, which
is `<hostString>/cmd`.

`void Mqtt_publish(const char* topic, const char* payload)`
Publishes any string message to the MQTT topic.

`void Mqtt_publish(const char* topic, float payload)`
Publishes float value (e.g. sensor data) to the MQTT topic.

`void Mqtt_setCallback(void(*callback)(char*, uint8_t*, unsigned int))`
Sets up the function which is being run when there is a new message on any
of the subscribed topics.

`void Mqtt_sendCrash(const char* topic)`
Sends recorded crash data to the MQTT server, line by line.

### NeoPixel.h
Is responsible for the NeoPixel strip.

#### Available functions
`void NeoPixel_setup(uint8_t pnum)`
Sets up a NeoPixel strip with a pnum number of LEDs (up to 16).

`void NeoPixel_progress(uint8_t level, uint32_t color)`
Shows progress: sets "level" number of LEDs to the color defined, rest is black.

`void NeoPixel_color(uint32_t color)`
Turns on every LED in the strip to the defined color.

`void NeoPixel_error(uint16_t errorCode)`
Shows error code using NeoPixel strip.

`void NeoPixel_anim(Anim type, uint32_t color)`
Shows indefinite animation of the defined type.

`void NeoPixel_animtime(Anim type, uint32_t color, uint16_t time)`
Shows timed animation of the defined type.

#### Animations and colors

There are three animations built-in: KnightRider1, KnightRider2 and RainbowCycle
where the first ones are the cylon-visor or KITT's visor moving from one side
to another, and RainbowCycle is just showing different colors.

There are built-in color constants for easier development: BrightRed, Black,
Green, Lime and Orange.

### OTA.h
Is responsible for the Over-The-Air (OTA) updates.

Update system, when run, will try to connect using HTTP to the server defined
in the configuration and download file from the address defined in the config
including version file to be substituted.

For example:

```cpp
// config->updateServer is example.com
// config->updatePort is 80
// config->updatePath is /firmware/%s.bin

handleOTA(config, "1.0"); // will download file from http://example.com:80/firmware/1.0.bin
```

There is no message something is wrong with OTA update apart from INFO level
message.

Due to [issue #1017](https://github.com/esp8266/Arduino/issues/1017) automatic
restart after update may not work.

#### Available functions

`void handleOTA(Configuration config, const char* version)`
Runs the OTA update based on the configuration to the specified version.

### SpiffsConfig.h
Is handling saving and reading config from `config.json` file. Configuration for
the application is saved in the SPIFFS in `data/` directory, in the file
`config.json`.

Configuration is used to set MQTT broker, port, OTA update server address and
similar information.

The configuration system is only for internal elements of Mokosh.

### WifiConnect.h
Is responsible for connecting to the wireless network.

#### Available functions

`bool WiFi_connect(const char* ssid, const char* password, const char* prefix)`
Connects to the Wi-Fi network defined by SSID and WPA2 password, settings the
hostname to `PREFIX_XXXXX`.

`char* WiFi_getHostString()`
Returns internal chip ID.
