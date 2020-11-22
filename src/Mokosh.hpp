#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

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

class Mokosh {
   public:
    // Constructor

    Mokosh();
    void setConfiguration(MokoshConfiguration config);  // przed begin, jeśli trzeba
    void setDebugLevel(DebugLevel level);                  // przed begin, jeśli trzeba
    void begin(String prefix);
    void loop();

    void publish(const char* subtopic, String payload);
    void publish(const char* subtopic, const char* payload);
    void publish(const char* subtopic, float payload);

    PubSubClient* getPubSubClient();

    void enableOTA();       // włącza obsługę polecenia OTA
    void disableFS();       // wyłącza obsługę LittleFS
    void enableFirstRun();  // włącza tryb first run jeżeli nie ma konfiguracji
    void enableRebootOnError(); 

    void onCommand(f_command_handler_t handler);        // przed begin - callback który ma być odpalany na customową komendę
    void onError(f_error_handler_t handler);            // przed begin - callback który ma być odpalany na wypadek błędu
    void onInterval(f_interval_t func, uint32_t time);  // uruchom funkcję f co czas time

    static Mokosh* getInstance();
    void factoryReset();

    void mqttCommandReceived(char* topic, uint8_t* message, unsigned int length);

    const uint8_t Error_CONFIG = 1;
    const uint8_t Error_FS = 2;
    const uint8_t Error_WIFI = 3;
    const uint8_t Error_BROKER = 4;
    const uint8_t Error_MQTT = 5;
    const uint8_t Error_NOTIMPLEMENTED = 6;

   private:
    f_error_handler_t errorHandler;
    f_command_handler_t commandHandler;
    bool debugReady;

    String hostName;
    char hostNameC[32];
    String prefix;
    MokoshConfiguration config;
    bool isConfigurationSet();

    WiFiClient* client;
    PubSubClient* mqtt;    

    bool isOtaEnabled = false;
    bool isFirstRunEnabled = false;
    bool isFSEnabled = true;
    bool isRebootOnError = false;

    DebugLevel debugLevel = DebugLevel::WARNING;

    bool configExists();
    bool configLoad();
    bool connectWifi();
    bool reconnect();

    const String cmd_topic = "cmd";
    const String version_topic = "version";
    const String debug_topic = "debug";
    const String heartbeat_topic = "debug/heartbeat";    

    void handleOta(char* version);

    void error(int code);

    char ssid[16] = {0};
    char version[32] = {0};
    char informationalVersion[100] = {0};
    char buildDate[11] = {0};
};

MokoshConfiguration create_configuration(const char* ssid, const char* password, const char* broker, uint16_t brokerPort, const char* updateServer,  uint16_t updatePort, const char* updatePath);