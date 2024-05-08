#if !defined(MOKOSH)

#define MOKOSH

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <ESP8266mDNS.h>
#endif

#if defined(ESP32)
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WiFiMulti.h>
#endif

#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <RemoteDebug.h>
#include <TickTwo.h>
#include <vector>

#include "MokoshConfig.hpp"
#include "MokoshHandlers.hpp"

// Debug level - starts from 0 to 6, higher is more severe
typedef enum DebugLevel
{
    PROFILER = 0,
    VERBOSE = 1,
    DEBUG = 2,
    INFO = 3,
    WARNING = 4,
    ERROR = 5,
    ANY = 6
} DebugLevel;

#define EVENTS_COUNT 10
#define HEARTBEAT 10000
#define MINUTES 60000
#define SECONDS 1000
#define HOURS 360000

#define mdebug(lvl, fmt, ...) Mokosh::debug(lvl, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mdebugA(fmt, ...) Mokosh::debug(DebugLevel::ANY, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mdebugE(fmt, ...) Mokosh::debug(DebugLevel::ERROR, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mdebugI(fmt, ...) Mokosh::debug(DebugLevel::INFO, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mdebugD(fmt, ...) Mokosh::debug(DebugLevel::DEBUG, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mdebugV(fmt, ...) Mokosh::debug(DebugLevel::VERBOSE, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mdebugW(fmt, ...) Mokosh::debug(DebugLevel::WARNING, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

enum MokoshErrors
{
    // error code thrown when config.json file cannot be read properly
    ConfigurationError = 1,

    // error code thrown when LittleFS is not available
    FileSystemNotAvailable = 2,

    // error code thrown when couldn't connect to Wi-Fi
    CannotConnectToWifi = 3,

    // error code thrown when MQTT broker connection fails
    MqttConnectionFailed = 5
};

// the main class for the framework, must be initialized in your code
class Mokosh
{
public:
    Mokosh();
    // sets debug level verbosity, must be called before begin()
    Mokosh *setDebugLevel(DebugLevel level);

    // starts Mokosh system, connects to the Wi-Fi and MQTT
    // using the provided device prefix
    void begin(String prefix, bool autoconnect = true);

    // handles MQTT, interval and RemoteDebug loops, must be called
    // in loop()
    void loop();

    // sets build information (SemVer and build date) used in the
    // responses to getv and getfullver commands and hello message
    // must be called before begin()
    Mokosh *setBuildMetadata(String version, String buildDate = "1970-01-01");

    // enables MDNS service (default false, unless OTA is enabled)
    Mokosh *setMDNS(bool value);

    // sets up the MDNS responder -- ran automatically unless custom
    // client is used
    void setupMDNS();

    // adds broadcasted MDNS service
    void addMDNSService(const char *service, const char *proto, uint16_t port);

    // adds broadcasted MDNS service props
    void addMDNSServiceProps(const char *service, const char *proto, const char *property, const char *value);

    // publishes a new message on a Prefix_ABCDE/subtopic topic with
    // a given payload
    void publish(const char *subtopic, String payload);

    // publishes a new message on a Prefix_ABCDE/subtopic topic with
    // a given payload
    void publish(const char *subtopic, const char *payload);

    // publishes a new message on a Prefix_ABCDE/subtopic topic with
    // a given payload, allows to specify if payload should be retained
    void publish(const char *subtopic, const char *payload, boolean retained);

    // publishes a new message on a Prefix_ABCDE/subtopic topic with
    // a given payload
    void publish(const char *subtopic, float payload);

    // returns internal PubSubClient instance
    PubSubClient *getPubSubClient();

    // prints message of a desired debug level to RemoteDebug
    // uses func parameter to be used with __func__ so there will
    // be printed in what function debug happened
    // use rather mdebug() macros instead
    static void debug(DebugLevel level, const char *func, const char *file, int line, const char *fmt, ...);

    // enables ArduinoOTA subsystem (disabled by default)
    // must be called before begin()
    Mokosh *setOta(bool value);

    // disables LittleFS and config.json support (enabled by default)
    // must be called before begin()
    Mokosh *setConfigFile(bool value);

    // enables FirstRun subsystem if there is no config.json (disabled by default)
    Mokosh *setFirstRun(bool value);

    // enables automatic reboot on error - by default there will be
    // an inifinite loop instead
    Mokosh *setRebootOnError(bool value);

    // defines callback to be run when command not handled by internal
    // means is received
    // must be set before begin()
    THandlerFunction_Command onCommand;

    // defines callback to be run when message is received to a subscribed
    // topic
    THandlerFunction_Message onMessage;

    // defines callback to be run when error is thrown
    // must be set before begin()
    THandlerFunction_MokoshError onError;

    // defines callback function func to be run on a specific time interval
    // Mokosh will automatically fire the function when time (in milliseconds)
    // occurs since the last run
    // you can also use TickTwo manually, by accessing vector tickers
    void registerIntervalFunction(fptr func, unsigned long time);

    // throws an error of a given code
    void error(int code);

    // returns instance of Mokosh singleton
    static Mokosh *getInstance();

    // removes config.json and reboots, entering FirstRun mode
    void factoryReset();

    // initializes FS and loads config file
    // is done automatically by begin(), but it can be run earlier
    // if access to config is needed before begin()
    void initializeFS();

    // the name of subtopic used for commands
    const char *cmd_topic = "cmd";

    // the name of subtopic used for version hello message
    const char *version_topic = "version";

    // the name of subtopic used for debug purposes
    const char *debug_topic = "debug";

    // the name of subtopic used for hello packet with IP
    const char *debug_ip_topic = "debug/ip";

    // the name of subtopic used as generic response to commands
    const char *debug_response_topic = "debug/cmdresp";

    // the name of subtopic used for heartbeat messages
    const char *heartbeat_topic = "debug/heartbeat";

    // this is a PRIVATE function, should not be used from the external code
    // exposed only as a workaround
    void _mqttCommandReceived(char *topic, uint8_t *message, unsigned int length);

    // this is a PRIVATE function, exposed only as a workaround
    void _processCommand(String command);

    // event handlers for OTA situations (onStart, onEnd, etc.)
    MokoshOTAHandlers otaEvents;

    // event handlers for Wi-Fi situations (onConnect, onDisconnect)
    MokoshWiFiHandlers wifiEvents;

    // handlers for additional events
    MokoshEvents events;

    // sets ignoring connection errors - useful in example of deep sleep
    // so the device is going to sleep again if wifi networks/mqtt are not
    // available
    Mokosh *setIgnoreConnectionErrors(bool value);

    // sets if the Wi-Fi should be reconnected on MQTT reconnect if needed
    Mokosh *setForceWiFiReconnect(bool value);

    // sets if the heartbeat messages should be send
    Mokosh *setHeartbeat(bool value);

    // sets if the IP message on hello should be retained
    // e.g. on Scaleway retained flag forces disconnect of the client
    Mokosh *setIPRetained(bool value);

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
    Mokosh *setCustomClient(Client &client);

    // a configuration object to set and read configs
    MokoshConfig config;

    // returns defined device prefix
    String getPrefix();

    // returns automatically generated device hostname
    String getHostName();

    // gets prefix for MQTT topics for the current device
    String getMqttPrefix();

    // connects to Wi-Fi, manually
    wl_status_t connectWifi();

    // returns used Wi-Fi (or custom) client
    Client *getClient();

    // vector of tickers, functions run on the given interval
    std::vector<std::shared_ptr<TickTwo>> getTickers();

    // registers a function that will run in a timeout, be default it will be run
    // one time (one-shot), and time tracking starts immediately
    void registerTimeoutFunction(fptr func, unsigned long time, int runs = 1, bool start = true);

private:
    bool debugReady;
    String hostName;
    char hostNameC[32];
    String prefix;
    String version = "1.0.0";
    String buildDate = "1970-01-01";

    Client *client;
    PubSubClient *mqtt;

    bool isFSEnabled = true;
    bool isRebootOnError = false;
    bool isOTAEnabled = false;
    bool isMDNSEnabled = false;
    bool isOTAInProgress = false;
    bool isMqttConfigured = false;
    bool isIgnoringConnectionErrors = false;
    bool isForceWifiReconnect = false;
    bool isWifiConfigured = false;
    bool isHeartbeatEnabled = true;
    bool isAfterBegin = false;
    bool isIPRetained = true;
    wl_status_t lastWifiStatus;

    DebugLevel debugLevel = DebugLevel::WARNING;

    bool configFileExists();
    bool isConfigurationSet();
    bool configureMqttClient();
    bool reconnect();

    void publishShortVersion();
    void publishIP();

    // initialization of tickers, is called automatically by begin()
    void initializeTickers();

    char ssid[16] = {0};

    std::vector<std::shared_ptr<TickTwo>> tickers;
};

namespace MokoshResilience
{

    // class for retrying things with delay between trials
    class Retry
    {
    public:
        // will retry the operation if returned false, each time delaying it
        // a bit longer, until the name of trials is being exceeded
        static bool retry(std::function<bool(void)> operation, int trials = 3, long delayTime = 100, int delayFactor = 2)
        {
            int i = 0;
            while (i < trials)
            {
                if (operation())
                    return true;

                long time = delayTime * power(delayFactor, i + 1);
                mdebugV("Resilience operation failed, retrying in %d", time);
                delay(time);
                i++;
            }

            mdebugV("Resilience operation failed after %d trials, giving up!", trials);
            return false;
        }

    private:
        static long power(int base, int exponent)
        {
            if (exponent == 0)
            {
                return 1;
            }

            long result = base;
            for (int i = 1; i < exponent; i++)
            {
                result *= base;
            }
            return result;
        }
    };

}

#endif