#include <Arduino.h>
#include <ArduinoJson.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <RemoteDebug.h>

#include "MokoshConfig.hpp"
#include "MokoshOTAConfig.hpp"

// handler for errors, used in onError
using f_error_handler_t = void (*)(int);

// handler for commands, used in onCommand
using f_command_handler_t = void (*)(uint8_t*, unsigned int);

// handler for interval functions, used in onInterval
using f_interval_t = void (*)();

// Debug level - starts from 0 to 6, higher is more severe
typedef enum DebugLevel {
    PROFILER = 0,
    VERBOSE = 1,
    DEBUG = 2,
    INFO = 3,
    WARNING = 4,
    ERROR = 5,
    ANY = 6
} DebugLevel;

typedef struct IntervalEvent {
    unsigned long interval;
    unsigned long last;
    f_interval_t handler;
} IntervalEvent;

#define EVENTS_COUNT 10
#define HEARTBEAT 10000
#define MINUTES 60000

#define mdebug(lvl, fmt, ...) Mokosh::debug(lvl, __func__, fmt, ##__VA_ARGS__)
#define mdebugA(fmt, ...) Mokosh::debug(DebugLevel::ANY, __func__, fmt, ##__VA_ARGS__)
#define mdebugE(fmt, ...) Mokosh::debug(DebugLevel::ERROR, __func__, fmt, ##__VA_ARGS__)
#define mdebugI(fmt, ...) Mokosh::debug(DebugLevel::INFO, __func__, fmt, ##__VA_ARGS__)
#define mdebugD(fmt, ...) Mokosh::debug(DebugLevel::DEBUG, __func__, fmt, ##__VA_ARGS__)
#define mdebugV(fmt, ...) Mokosh::debug(DebugLevel::VERBOSE, __func__, fmt, ##__VA_ARGS__)
#define mdebugW(fmt, ...) Mokosh::debug(DebugLevel::WARNING, __func__, fmt, ##__VA_ARGS__)

// the main class for the framework, must be initialized in your code
class Mokosh {
   public:
    Mokosh();
    // sets debug level verbosity, must be called before begin()
    void setDebugLevel(DebugLevel level);

    // starts Mokosh system, connects to the Wi-Fi and MQTT
    // using the provided device prefix
    void begin(String prefix);

    // handles MQTT, interval and RemoteDebug loops, must be called
    // in loop()
    void loop();

    // sets build information (SemVer and build date) used in the
    // responses to getv and getfullver commands and hello message
    // must be called before begin()
    void setBuildMetadata(String version, String buildDate);

    // publishes a new message on a Prefix_ABCDE/subtopic topic with
    // a given payload
    void publish(const char* subtopic, String payload);

    // publishes a new message on a Prefix_ABCDE/subtopic topic with
    // a given payload
    void publish(const char* subtopic, const char* payload);

    // publishes a new message on a Prefix_ABCDE/subtopic topic with
    // a given payload, allows to specify if payload should be retained
    void publish(const char* subtopic, const char* payload, boolean retained);

    // publishes a new message on a Prefix_ABCDE/subtopic topic with
    // a given payload
    void publish(const char* subtopic, float payload);

    // returns internal PubSubClient instance
    PubSubClient* getPubSubClient();

    // prints message of a desired debug level to RemoteDebug
    // uses func parameter to be used with __func__ so there will
    // be printed in what function debug happened
    // use rather mdebug() macros instead
    static void debug(DebugLevel level, const char* func, const char* fmt, ...);

    // enables ArduinoOTA subsystem
    // must be called before begin()
    void enableOTA();

    // disables LittleFS and config.json support
    // must be called before begin()
    void disableFS();

    // enables FirstRun subsystem if there is no config.json
    void enableFirstRun();

    // enables automatic reboot on error - by default there will be
    // an inifinite loop instead
    void enableRebootOnError();

    // defines callback to be run when command not handled by internal
    // means is received
    // must be called before begin()
    void onCommand(f_command_handler_t handler);

    // defines callback to be run when error is thrown
    // must be called before begin()
    void onError(f_error_handler_t handler);

    // defines callback function func to be run on a specific time interval
    // Mokosh will automatically fire the function when time (in milliseconds)
    // occurs since the last run
    // must be called before begin()
    void onInterval(f_interval_t func, unsigned long time);

    // throws an error of a given code
    void error(int code);

    // returns instance of Mokosh singleton
    static Mokosh* getInstance();

    // removes config.json and reboots, entering FirstRun mode
    void factoryReset();

    // error code thrown when config.json file cannot be read properly
    const uint8_t Error_CONFIG = 1;

    // error code thrown when LittleFS is not available
    const uint8_t Error_FS = 2;

    // error code thrown when couldn't connect to Wi-Fi
    const uint8_t Error_WIFI = 3;

    // error code thrown when MQTT broker connection fails
    const uint8_t Error_MQTT = 5;

    // the name of subtopic used for commands
    const String cmd_topic = "cmd";

    // the name of subtopic used for version hello message
    const String version_topic = "version";

    // the name of subtopic used for debug purposes
    const String debug_topic = "debug";

    // the name of subtopic used for hello packet with IP
    const String debug_ip_topic = "debug/ip";

    // the name of subtopic used as generic response to commands
    const String debug_response_topic = "debug/cmdresp";

    // the name of subtopic used for heartbeat messages
    const String heartbeat_topic = "debug/heartbeat";

    const String broker_field = "broker";
    const String broker_port_field = "brokerPort";
    const String ota_port_field = "otaPort";
    const String ssid_field = "ssid";
    const String wifi_password_field = "password";

    // reads a given string field from config.json
    String readConfigString(const char* field, String def = "");

    // reads a given int field from config.json
    int readConfigInt(const char* field, int def = 0);

    // reads a given float field from config.json
    float readConfigFloat(const char* field, float def = 0);

    // sets a configuration field to a given value
    void setConfig(const char* field, String value);

    // sets a configuration field to a given value
    void setConfig(const char* field, int value);

    // sets a configuration field to a given value
    void setConfig(const char* field, float value);

    // saves configuration to a config.json file
    void saveConfig();

    // reads configuration from a config.json file
    bool reloadConfig();

    void mqttCommandReceived(char* topic, uint8_t* message, unsigned int length);

    // property with all possible OTA parameters
    MokoshOTAConfiguration OTA;

    // sets ignoring connection errors - useful in example of deep sleep
    // so the device is going to sleep again if wifi networks/mqtt are not
    // available
    void setIgnoreConnectionErrors(bool value);

    // sets if the Wi-Fi should be reconnected on MQTT reconnect if needed
    void setForceWiFiReconnect(bool value);

    // returns if the RemoteDebug is ready
    bool isDebugReady();

    // returns if Wi-Fi is connected at all
    bool isWifiConnected();

   private:
    f_error_handler_t errorHandler;
    f_command_handler_t commandHandler;

    bool debugReady;
    String hostName;
    char hostNameC[32];
    String prefix;    
    IntervalEvent events[EVENTS_COUNT];
    String version = "1.0.0";
    String buildDate = "1970-01-01";

    StaticJsonDocument<500> config;

    WiFiClient* client;
    PubSubClient* mqtt;

    bool isFSEnabled = true;
    bool isRebootOnError = false;
    bool isOTAEnabled = false;
    bool isOTAInProgress = false;
    bool isIgnoringConnectionErrors = false;
    bool isForceWifiReconnect = false;

    DebugLevel debugLevel = DebugLevel::WARNING;

    bool configFileExists();
    bool isConfigurationSet();
    bool connectWifi();
    bool configureMqttClient();
    bool reconnect();

    void publishShortVersion();
    void publishIP();

    char ssid[16] = {0};
};