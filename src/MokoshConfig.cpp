#include "MokoshConfig.hpp"
#include "Mokosh.hpp"

#if defined(NRF52) || defined(NRF52840_XXAA)
#include <Adafruit_LittleFS.h>
Adafruit_LittleFS LittleFS;

using namespace Adafruit_LittleFS_Namespace;
#endif

#if defined(ARDUINO_RASPBERRY_PI_PICO)
#include <LittleFS.h>
#endif

const char *MokoshConfig::KEY = "CONFIG";

MokoshConfig::MokoshConfig(bool useFileSystem)
{
    this->useFileSystem = useFileSystem;
}

bool MokoshConfig::isConfigurationSet()
{
    return this->get<String>("ssid") != "";
}

bool MokoshConfig::configFileExists()
{
#if defined(NRF52) || defined(NRF52840_XXAA)
    File configFile = LittleFS.open("/config.json", Adafruit_LittleFS_Namespace::FILE_O_READ);
#elif defined(ESP8266) || defined(ESP32)
    File configFile = LittleFS.open("/config.json", "r");
#elif defined(ARDUINO_RASPBERRY_PI_PICO)
    File configFile = LittleFS.open("/config.json", "r");
#endif

    if (!configFile)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void MokoshConfig::set(const char *field, String value)
{
    this->config[field] = value;
}

void MokoshConfig::set(const char *field, const char *value)
{
    this->config[field] = value;
}

void MokoshConfig::set(const char *field, int value)
{
    this->config[field] = value;
}

void MokoshConfig::set(const char *field, float value)
{
    this->config[field] = value;
}

void MokoshConfig::saveConfig()
{
    if (!this->useFileSystem)
        return;

    mlogV("Saving config.json");
#if defined(NRF52) || defined(NRF52840_XXAA)
    File configFile = LittleFS.open("/config.json", Adafruit_LittleFS_Namespace::FILE_O_WRITE);
#elif defined(ESP8266) || defined(ESP32)
    File configFile = LittleFS.open("/config.json", "w");
#elif defined(ARDUINO_RASPBERRY_PI_PICO)
    File configFile = LittleFS.open("/config.json", "w");
#endif

    serializeJson(this->config, configFile);
}

bool MokoshConfig::reloadFromFile()
{
    if (!this->useFileSystem)
        return true;

    mlogV("Reloading config.json");
#if defined(NRF52) || defined(NRF52840_XXAA)
    File configFile = LittleFS.open("/config.json", Adafruit_LittleFS_Namespace::FILE_O_READ);
#elif defined(ESP8266) || defined(ESP32)
    File configFile = LittleFS.open("/config.json", "r");
#elif defined(ARDUINO_RASPBERRY_PI_PICO)
    File configFile = LittleFS.open("/config.json", "r");
#endif

    if (!configFile)
    {
        mlogE("Cannot open config.json file");
        return false;
    }

    size_t size = configFile.size();
    if (size > 1000)
    {
        mlogE("Config file too large");
        return false;
    }

    deserializeJson(this->config, configFile);

    return true;
}

bool MokoshConfig::hasKey(const char *field)
{
    return this->config.containsKey(field);
}

void MokoshConfig::removeConfigFile()
{
    if (!this->useFileSystem)
        return;

    mlogV("Removing config.json");
    LittleFS.remove("/config.json");
}

// sets up the configuration system
bool MokoshConfig::setup()
{
    if (!this->useFileSystem)
        return true;

    if (!LittleFS.begin())
    {
        if (!LittleFS.format())
            return false;
    }

    if (this->useFileSystem)
        this->reloadFromFile();

    this->setupFinished = true;

    return true;
}

void MokoshConfig::loop()
{
}

bool MokoshConfig::command(String command, String param)
{
    if (command == "saveconfig")
    {
        mlogD("Config saved");
        this->saveConfig();
        return true;
    }

    if (command == "showconfigs")
    {
        String value = this->get<String>(param.c_str());
        mlogD("config %s = %s", param.c_str(), value.c_str());
        return true;
    }

    if (command == "showconfigi")
    {
        int value = this->get<int>(param.c_str());
        mlogD("config %s = %i", param.c_str(), value);
        return true;
    }

    if (command == "showconfigf")
    {
        float value = this->get<float>(param.c_str());
        mlogD("config %s = %f", param.c_str(), value);
        return true;
    }

    if (command == "setconfigs")
    {
        String field = param.substring(0, param.indexOf('|'));
        String value = param.substring(param.indexOf('|') + 1);

        mlogD("Setting configuration: field: %s, new value: %s", field.c_str(), value.c_str());

        this->set(field.c_str(), value);

        return true;
    }

    if (command == "setconfigi")
    {
        String field = param.substring(0, param.indexOf('|'));
        String value = param.substring(param.indexOf('|') + 1);

        mlogD("Setting configuration: field: %s, new value: %d", field.c_str(), value.toInt());

        this->set(field.c_str(), (int)(value.toInt()));

        return true;
    }

    if (command == "setconfigf")
    {
        String field = param.substring(0, param.indexOf('|'));
        String value = param.substring(param.indexOf('|') + 1);

        mlogD("Setting configuration: field: %s, new value: %f", field.c_str(), value.toFloat());

        this->set(field.c_str(), value.toFloat());

        return true;
    }

    if (command == "reloadconfig")
    {
        mlogI("Config reload initiated");
        this->reloadFromFile();

        return true;
    }

    return false;
}