#include "MokoshConfig.hpp"
#include "Mokosh.hpp"

bool MokoshConfig::isConfigurationSet()
{
    return this->get<String>("ssid") != "";
}

bool MokoshConfig::configFileExists()
{
    File configFile = LittleFS.open("/config.json", "r");

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

void MokoshConfig::save()
{
    mdebugV("Saving config.json");
    File configFile = LittleFS.open("/config.json", "w");
    serializeJson(this->config, configFile);
}

bool MokoshConfig::reload()
{
    mdebugV("Reloading config.json");
    File configFile = LittleFS.open("/config.json", "r");

    if (!configFile)
    {
        mdebugE("Cannot open config.json file");
        return false;
    }

    size_t size = configFile.size();
    if (size > 500)
    {
        mdebugE("Config file too large");
        return false;
    }

    deserializeJson(this->config, configFile);

    return true;
}

bool MokoshConfig::hasKey(const char *field)
{
    return this->config.containsKey(field);
}

void MokoshConfig::removeFile()
{
    mdebugV("Removing config.json");
    LittleFS.remove("/config.json");
}

bool MokoshConfig::prepareFS()
{
    if (!LittleFS.begin())
    {
        if (!LittleFS.format())
            return false;
    }

    return true;
}