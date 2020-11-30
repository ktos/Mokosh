#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <RemoteDebug.h>

#include "MokoshConfig.hpp"

using f_error_handler_t = void (*)(int);
using f_command_handler_t = void (*)(uint8_t*, unsigned int);
using f_interval_t = void (*)();

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

class Mokosh {
   public:
    Mokosh();
    void setConfiguration(MokoshConfiguration config);  // przed begin, jeśli trzeba
    void setDebugLevel(DebugLevel level);               // przed begin, jeśli trzeba
    void begin(String prefix);
    void loop();

    void publish(const char* subtopic, String payload);
    void publish(const char* subtopic, const char* payload);
    void publish(const char* subtopic, float payload);

    PubSubClient* getPubSubClient();

    void debug(DebugLevel level, char* func, char* fmt, ...);

    void enableOTA();       // włącza obsługę polecenia OTA
    void disableFS();       // wyłącza obsługę LittleFS
    void enableFirstRun();  // włącza tryb first run jeżeli nie ma konfiguracji
    void enableRebootOnError();

    void onCommand(f_command_handler_t handler);             // przed begin - callback który ma być odpalany na customową komendę
    void onError(f_error_handler_t handler);                 // przed begin - callback który ma być odpalany na wypadek błędu
    void onInterval(f_interval_t func, unsigned long time);  // uruchom funkcję f co czas time
    void error(int code);

    static Mokosh* getInstance();    
    void factoryReset();

    void mqttCommandReceived(char* topic, uint8_t* message, unsigned int length);

    const uint8_t Error_CONFIG = 1;
    const uint8_t Error_FS = 2;
    const uint8_t Error_WIFI = 3;
    const uint8_t Error_BROKER = 4;
    const uint8_t Error_MQTT = 5;
    const uint8_t Error_NOTIMPLEMENTED = 6;

    const String cmd_topic = "cmd";
    const String version_topic = "version";
    const String debug_topic = "debug";
    const String heartbeat_topic = "debug/heartbeat";

    static MokoshConfiguration CreateConfiguration(const char* ssid, const char* password, const char* broker, uint16_t brokerPort);
    static void debug(DebugLevel level, const char* func, const char* fmt, ...);    

   private:
    f_error_handler_t errorHandler;
    f_command_handler_t commandHandler;
    bool debugReady;

    String hostName;
    char hostNameC[32];
    String prefix;
    MokoshConfiguration config;
    IntervalEvent events[EVENTS_COUNT];

    WiFiClient* client;
    PubSubClient* mqtt;

    bool isFSEnabled = true;
    bool isRebootOnError = false;

    DebugLevel debugLevel = DebugLevel::WARNING;

    bool configExists();
    bool isConfigurationSet();
    bool configLoad();
    bool connectWifi();
    bool reconnect();

    void startOTAUpdate(char* version);    

    char ssid[16] = {0};
    char version[32] = {0};
    char informationalVersion[100] = {0};
    char buildDate[11] = {0};
};