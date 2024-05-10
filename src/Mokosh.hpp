#if !defined(MOKOSH)

#define MOKOSH

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#endif

#if defined(ESP32)
#include <LittleFS.h>
#include <WiFiMulti.h>
#endif

#include <PubSubClient.h>
#include <RemoteDebug.h>
#include <TickTwo.h>
#include <vector>
#include <map>
#include <algorithm>

#include "MokoshConfig.hpp"
#include "MokoshHandlers.hpp"
#include "MokoshService.hpp"

// Debug level - starts from 0 to 6, higher is more severe
typedef enum LogLevel
{
    PROFILER = 0,
    VERBOSE = 1,
    DEBUG = 2,
    INFO = 3,
    WARNING = 4,
    ERROR = 5,
    ANY = 6
} LogLevel;

#define EVENTS_COUNT 10
#define HEARTBEAT 10000
#define MINUTES 60000
#define SECONDS 1000
#define HOURS 360000

#define mdebug(lvl, fmt, ...) Mokosh::debug(lvl, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mdebugA(fmt, ...) Mokosh::debug(LogLevel::ANY, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mdebugE(fmt, ...) Mokosh::debug(LogLevel::ERROR, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mdebugI(fmt, ...) Mokosh::debug(LogLevel::INFO, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mdebugD(fmt, ...) Mokosh::debug(LogLevel::DEBUG, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mdebugV(fmt, ...) Mokosh::debug(LogLevel::VERBOSE, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define mdebugW(fmt, ...) Mokosh::debug(LogLevel::WARNING, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

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
    // creates a new instance of a Mokosh framework object
    // optionally sets up config file - if not set, in RAM will be used
    Mokosh(String prefix = "Mokosh", String version = "1.0.0", bool useFilesystem = true);

    // sets debug level verbosity, must be called before begin()
    Mokosh *setLogLevel(LogLevel level);

    // starts Mokosh system, connects to the Wi-Fi and MQTT
    // using the provided device prefix, sets up all registered services
    void begin(bool autoconnect = true);

    // handles MQTT, interval and RemoteDebug loops, must be called
    // in loop()
    void loop();

    // registers a MokoshService
    // if called after begin(), the service will be set up immediately
    // and if not, in a setup phase of the services
    Mokosh *registerService(const char *key, std::shared_ptr<MokoshService> service);

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
    static void debug(LogLevel level, const char *func, const char *file, int line, const char *fmt, ...);

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
    std::shared_ptr<MokoshConfig> config;

    // returns defined device prefix
    String getPrefix();

    // returns automatically generated device hostname
    String getHostName();

    // gets prefix for MQTT topics for the current device
    String getMqttPrefix();

    // returns prefix and hostname combination which are used for device ident
    String getHostNameWithPrefix();

    // connects to Wi-Fi, manually
    wl_status_t connectWifi();

    // returns used Wi-Fi (or custom) client
    Client *getClient();

    // vector of tickers, functions run on the given interval
    std::vector<std::shared_ptr<TickTwo>> getTickers();

    // registers a function that will run in a timeout, be default it will be run
    // one time (one-shot), and time tracking starts immediately
    void registerTimeoutFunction(fptr func, unsigned long time, int runs = 1, bool start = true);

    // returns a version string registered before build()
    String getVersion();

    // sets up all registered services
    // will be run automatically during begin()
    void setupServices();

    // sets up a single service with dependency resolution
    bool setupService(const char *key, std::shared_ptr<MokoshService> service);

    // checks if the service with a given name is registered
    bool isServiceRegistered(const char *key);

    // returns a service by a name and downcast to the specified type
    template <typename T>
    std::shared_ptr<T> getRegisteredService(const char *key)
    {
        return std::static_pointer_cast<T>(this->services[key]);
    }

private:
    bool debugReady;
    String hostName;
    char hostNameC[32];
    String prefix;
    String version = "1.0.0";

    Client *client;
    PubSubClient *mqtt;

    bool isFSEnabled = true;
    bool isRebootOnError = false;
    bool isOTAEnabled = false;
    bool isOTAInProgress = false;
    bool isMqttConfigured = false;
    bool isIgnoringConnectionErrors = false;
    bool isForceWifiReconnect = false;
    bool isWifiConfigured = false;
    bool isHeartbeatEnabled = true;
    bool isAfterBegin = false;
    bool isIPRetained = true;
    wl_status_t lastWifiStatus;

    LogLevel debugLevel = LogLevel::WARNING;

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
    std::map<const char *, std::shared_ptr<MokoshService>> services;
};

#include "MokoshResilience.hpp"

#endif