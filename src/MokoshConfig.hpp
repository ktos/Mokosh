#if !defined(MOKOSH_CONFIG)

#define MOKOSH_CONFIG

#include <Arduino.h>
#include <ArduinoJson.h>

class MokoshConfig {
   public:
    const String config_broker = "broker";
    const String config_broker_port = "brokerPort";
    const String config_ota_port = "otaPort";
    const String config_ota_password = "otaPasswordHash";
    const String config_ssid = "ssid";
    const String config_wifi_password = "password";
    const String config_client_id = "mqttClientId";

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

    // checks if the key exists in configuration
    bool hasConfigKey(const char* field);

    // removes configuration file
    void removeConfigFile();

    // prepares file system
    bool prepareFS();

    // returns if configuration has been set
    bool isConfigurationSet();

    // returns if configuration file exists
    bool configFileExists();

   private:
    StaticJsonDocument<500> config;
};

#endif