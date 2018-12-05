# Mokosh

Mokosh is a pseudo-framework, extracted from the "Chione" project, with a goal
to be a simple set of a functions for easily built similarly operated IoT
devices.

Mokosh is built on the set of functions, not objects, and is mostly C-based,
instead of C++, sometimes relying on the global variables.

This version of Mokosh is heavily in development.

## Usage

Code is prepared for the Arduino platform, currently building on version 1.8.
Prepared only for ESP8266, tested on NodeMCU and WeMos D1 R2.

To build a Mokosh-based application, you have to define a few compilation flags:
* `ENABLE_NEOPIXEL` will enable showing errors using the NeoPixel strip,
* `DEBUG_LEVEL` will define verbosity levels for debug printing functions.

And you have to just include the main file somewhere in the beginning of your
Arduino sketch:

```cpp
#include <Mokosh.h>
```

Then you can just start using Mokosh functions.

### Debug Level
Serial output (by default 115200 kbps) verbosity is defined by `DEBUG_LEVEL`
constant, allowing values 1, 2, 4 or 8.

When the level is set to a certain value, all messages with level defined as
higher or equal are sent to a serial port. E.g.: `DEBUG_LEVEL 2` will allow
sending messages with level set to `DLVL_INFO` (2), `DLVL_WARNING` (4) and 
`DLVL_ERROR` (8), but not `DLVL_DEBUG` (1).

### SPIFFS Configuration
Configuration for the application is saved in the SPIFFS in `data/` directory,
int the file `config.json`.

Configuration is used to set MQTT broker, port, OTA update server address and
similar information.

## Structure

### Debug.h
Contains functions for sending debug messages to the serial port and debug
levels definitions.

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

### MQTT.h
Will communicate with a MQTT broker. By default the device will subscribe
the topic `PREFIX_XXXXX/cmd`, awaiting for commands. You have to provide
your own function responding to the commands using Mqtt_setCallback().

### NeoPixel.h
Is responsible for the NeoPixel strip.

### OTA.h
Is responsible for the Over-The-Air (OTA) updates.

Update system, when run, will try to connect using HTTP to the server defined
in the configuration and download file from the address defined in the config
including version file to be substituted.

For example:

```cpp
// config->updateServer is example.com
// config->updatePort is 80
// config->otaPath is /firmware/%s.bin

handleOTA(config, "1.0"); // will download file from http://example.com:80/firmware/1.0.bin
```

There is no message something is wrong with OTA update apart from INFO level
message.

Due to [issue #1017](https://github.com/esp8266/Arduino/issues/1017) automatic
restart after update may not work.

### SpiffsConfig.h
Is handling saving and reading config from `config.json` file.

### WifiConnect.h
Is responsible for connecting to the wireless network.