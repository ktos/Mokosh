#if !defined(MOKOSH)

#define MOKOSH

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#endif

#if defined(ESP32)
#include <ESPmDNS.h>
#include <SPIFFS.h>
#define LittleFS SPIFFS
#endif

#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <RemoteDebug.h>

#include "MokoshConfig.hpp"
#include "MokoshHandlers.hpp"

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
    String name;
    unsigned long interval;
    unsigned long last;
    THandlerFunction handler;
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

class MokoshErrors {
   public:
    // error code thrown when config.json file cannot be read properly
    static const uint8_t ConfigurationError = 1;

    // error code thrown when LittleFS is not available
    static const uint8_t FileSystemNotAvailable = 2;

    // error code thrown when couldn't connect to Wi-Fi
    static const uint8_t CannotConnectToWifi = 3;

    // error code thrown when MQTT broker connection fails
    static const uint8_t MqttConnectionFailed = 5;
};

// the main class for the framework, must be initialized in your code
class Mokosh {
   public:
    Mokosh();
    // sets debug level verbosity, must be called before begin()
    void setDebugLevel(DebugLevel level);

    // starts Mokosh system, connects to the Wi-Fi and MQTT
    // using the provided device prefix
    void begin(String prefix, bool autoconnect = true);

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
    void disableLoadingConfigFile();

    // enables FirstRun subsystem if there is no config.json
    void enableFirstRun();

    // enables automatic reboot on error - by default there will be
    // an inifinite loop instead
    void enableRebootOnError();

    // defines callback to be run when command not handled by internal
    // means is received
    // must be called before begin()
    THandlerFunction_Command onCommand;

    // defines callback to be run when error is thrown
    // must be called before begin()
    THandlerFunction_MokoshError onError;

    // defines callback function func to be run on a specific time interval
    // Mokosh will automatically fire the function when time (in milliseconds)
    // occurs since the last run
    // must be called before begin()
    void onInterval(THandlerFunction func, unsigned long time, String name = "");

    // throws an error of a given code
    void error(int code);

    // returns instance of Mokosh singleton
    static Mokosh* getInstance();

    // removes config.json and reboots, entering FirstRun mode
    void factoryReset();

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

    void mqttCommandReceived(char* topic, uint8_t* message, unsigned int length);

    // handlers for OTA situations (onStart, onEnd, etc.)
    MokoshOTAHandlers OtaHandlers;

    // handlers for Wi-Fi situations (onConnect, onDisconnect)
    MokoshWiFiHandlers WiFiHandlers;

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

    // sends "hello" packet with version number and sets up heartbeat
    // is being automatically run on begin() if autoconnect is true
    void hello();

    // sets up OTA in Wi-Fi networks
    // is being automatically run on begin() if autoconnect is true
    void setupOta();

    // sets up Wi-Fi client
    // is being automatically run on begin() if autoconnect is true
    void setupWiFiClient();

    // sets up MQTT client
    // is being automatically run on begin() if autoconnect is true
    void setupMqttClient();

    // sets up RemoteDebug
    // is being automatically run on begin() if autoconnect is true
    void setupRemoteDebug();

    // sets up communication using the custom Client instance (e.g. GSM)
    // remember to use at least setupMqttClient() and hello() after using
    // this, autoconnect should be disabled
    void setupCustomClient(Client& client);

   private:
    bool debugReady;
    String hostName;
    char hostNameC[32];
    String prefix;
    IntervalEvent events[EVENTS_COUNT];
    String version = "1.0.0";
    String buildDate = "1970-01-01";

    MokoshConfig config;

    Client* client;
    PubSubClient* mqtt;

    bool isFSEnabled = true;
    bool isRebootOnError = false;
    bool isOTAEnabled = false;
    bool isOTAInProgress = false;
    bool isMqttConfigured = false;
    bool isIgnoringConnectionErrors = false;
    bool isForceWifiReconnect = false;
    wl_status_t lastWifiStatus;

    DebugLevel debugLevel = DebugLevel::WARNING;

    bool configFileExists();
    bool isConfigurationSet();
    wl_status_t connectWifi();
    bool configureMqttClient();
    bool reconnect();

    void publishShortVersion();
    void publishIP();

    char ssid[16] = {0};
};

#endif