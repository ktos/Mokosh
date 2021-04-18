#include "MokoshConfig.hpp"
#include "Mokosh.hpp"

bool MokoshConfig::isConfigurationSet() {
    return this->getString("ssid") != "";
}

bool MokoshConfig::configFileExists() {
    File configFile = LittleFS.open("/config.json", "r");

    if (!configFile) {
        return false;
    } else {
        return true;
    }
}

String MokoshConfig::getString(const char* field, String def) {
    if (!this->config.containsKey(field)) {
        mdebugW("config.json field %s does not exist!", field);
        return def;
    }

    const char* data = this->config[field];
    return String(data);
}

int MokoshConfig::getInt(const char* field, int def) {
    if (!this->config.containsKey(field)) {
        mdebugW("config.json field %s does not exist!", field);
        return def;
    }

    int data = this->config[field];
    return data;
}

float MokoshConfig::getFloat(const char* field, float def) {
    if (!this->config.containsKey(field)) {
        mdebugW("config.json field %s does not exist!", field);
        return def;
    }

    float data = this->config[field];
    return data;
}

void MokoshConfig::set(const char* field, String value) {
    this->config[field] = value;
}

void MokoshConfig::set(const char* field, const char* value) {
    this->config[field] = value;
}

void MokoshConfig::set(const char* field, int value) {
    this->config[field] = value;
}

void MokoshConfig::set(const char* field, float value) {
    this->config[field] = value;
}

void MokoshConfig::save() {
    mdebugV("Saving config.json");
    File configFile = LittleFS.open("/config.json", "w");
    serializeJson(this->config, configFile);
}

bool MokoshConfig::reload() {
    mdebugV("Reloading config.json");
    File configFile = LittleFS.open("/config.json", "r");

    if (!configFile) {
        mdebugE("Cannot open config.json file");
        return false;
    }

    size_t size = configFile.size();
    if (size > 500) {
        mdebugE("Config file too large");
        return false;
    }

    deserializeJson(this->config, configFile);

    return true;
}

bool MokoshConfig::hasKey(const char* field) {
    return this->config.containsKey(field);
}

void MokoshConfig::removeFile() {
    mdebugV("Removing config.json");
    LittleFS.remove("/config.json");
}

bool MokoshConfig::prepareFS() {
    if (!LittleFS.begin()) {
        if (!LittleFS.format())
            return false;
    }

    return true;
}