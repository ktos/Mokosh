#if !defined(MOKOSH_CONFIG)

#define MOKOSH_CONFIG

#include <Arduino.h>
#include <ArduinoJson.h>

class MokoshConfig {
   public:
    const char* key_broker = "broker";
    const char* key_broker_port = "brokerPort";
    const char* key_ota_port = "otaPort";
    const char* key_ota_password = "otaPasswordHash";
    const char* key_ssid = "ssid";
    const char* key_wifi_password = "password";
    const char* key_client_id = "mqttClientId";

    // reads a given string field from config.json
    String getString(const char* field, String def = "");

    // reads a given int field from config.json
    int getInt(const char* field, int def = 0);

    // reads a given float field from config.json
    float getFloat(const char* field, float def = 0);

    // sets a configuration field to a given value
    void set(const char* field, String value);

    // sets a configuration field to a given value
    void set(const char* field, const char* value);

    // sets a configuration field to a given value
    void set(const char* field, int value);

    // sets a configuration field to a given value
    void set(const char* field, float value);

    // saves configuration to a config.json file
    void save();

    // reads configuration from a config.json file
    bool reload();

    // checks if the key exists in configuration
    bool hasKey(const char* field);

    // removes configuration file
    void removeFile();

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