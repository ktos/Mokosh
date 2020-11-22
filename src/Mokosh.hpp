#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "MokoshConfig.hpp"

using f_error_handler_t = void (*)(int);
using f_command_handler_t = void (*)(uint8_t*, unsigned int);
using f_interval_t = void (*)();

class Mokosh {
   public:
    // Constructor

    Mokosh();
    void setConfiguration(MokoshConfiguration config);  // przed begin, jeśli trzeba
    void setDebugLevel(uint8_t level);                  // przed begin, jeśli trzeba
    void begin(String prefix);
    void loop();

    void publish(const char* subtopic, String payload);
    void publish(const char* subtopic, const char* payload);
    void publish(const char* subtopic, float payload);

    PubSubClient& getPubSubClient();

    void enableOTA();       // włącza obsługę polecenia OTA
    void enableFirstRun();  // włącza tryb first run jezęli nie ma konfiguracji

    void onCommand(f_command_handler_t handler);        // przed begin - callback który ma być odpalany na customową komendę
    void onError(f_error_handler_t handler);            // przed begin - callback który ma być odpalany na wypadek błędu
    void onInterval(f_interval_t func, uint32_t time);  // uruchom funkcję f co czas time

    static Mokosh* getInstance();
    void factoryReset();

    const uint8_t Error_CONFIG = 1;
    const uint8_t Error_SPIFFS = 2;
    const uint8_t Error_WIFI = 3;
    const uint8_t Error_BROKER = 4;
    const uint8_t Error_MQTT = 5;

   private:
    f_error_handler_t errorHandler;
    f_command_handler_t commandHandler;

    String hostName;
    char hostNameC[32];
    String prefix;
    MokoshConfiguration* config;

    WiFiClient client;
    PubSubClient mqtt;

    IPAddress brokerAddress;
    uint16_t brokerPort;

    bool isOtaEnabled = false;
    bool isFirstRunEnabled = false;

    uint8_t debugLevel = 0;

    bool configExists();
    bool configLoad();
    bool connectWifi();
    bool reconnect();

    char cmd_topic[36];
    char version_topic[36];
    char debug_topic[36];
    char heartbeat_topic[42];

    void onCommand(char* topic, uint8_t* message, unsigned int length);

    void handleOta(char* version);

    void error(int code);

    char ssid[16] = {0};
    char version[32] = {0};
    char informationalVersion[100] = {0};
    char buildDate[11] = {0};
};