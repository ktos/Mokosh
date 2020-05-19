#include "SpiffsConfig.h"

#include <ArduinoJson.h>
#include <FS.h>

#include "Debug.h"
#include "NeoPixel.h"

bool SpiffsConfig_begin() {
    return SPIFFS.begin();
}

bool SpiffsConfig_exists() {
    File configFile = SPIFFS.open("/config.json", "r");

    if (!configFile) {
        return false;
    } else {
        return true;
    }
}

bool SpiffsConfig_load(MokoshConfiguration *config) {
    File configFile = SPIFFS.open("/config.json", "r");

    if (!configFile) {
        Debug_print(DLVL_ERROR, "CONFIG", "Cannot open config file");
        return false;
    }

    size_t size = configFile.size();
    if (size > 1024) {
        Debug_print(DLVL_ERROR, "CONFIG", "Config file too large");
        return false;
    }

    // Allocate a buffer to store contents of the file.
    std::unique_ptr<char[]> buf(new char[size]);

    // We don't use String here because ArduinoJson library requires the input
    // buffer to be mutable. If you don't use ArduinoJson, you may as well
    // use configFile.readString instead.
    configFile.readBytes(buf.get(), size);

    StaticJsonBuffer<300> jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject(buf.get());

    if (!json.success()) {
        Debug_print(DLVL_ERROR, "CONFIG", "Failed to parse config file");
        return false;
    }

    const char *ssid2 = json["ssid"];
    const char *password2 = json["password"];
    const char *broker2 = json["broker"];
    const char *update2 = json["updateServer"];

    memcpy(config->ssid, ssid2, 32);
    memcpy(config->password, password2, 32);
    memcpy(config->broker, broker2, 32);
    memcpy(config->updateServer, update2, 32);

    config->brokerPort = json["brokerPort"];
    config->updatePort = json["updatePort"];

    const char *color = json["color"];
    String color2 = color;

    if (color2.length() == 11) {
        int r = color2.substring(0, 3).toInt();
        int g = color2.substring(4, 7).toInt();
        int b = color2.substring(8, 11).toInt();

        config->color = NeoPixel_convertColorToCode(r, g, b);
    } else {
        config->color = (uint32_t)json["color"];
    }

    Debug_print(DLVL_DEBUG, "CONFIG", config->ssid);
    Debug_print(DLVL_DEBUG, "CONFIG", config->password);
    Debug_print(DLVL_DEBUG, "CONFIG", config->broker);
    Debug_print(DLVL_DEBUG, "CONFIG", config->brokerPort);
    Debug_print(DLVL_DEBUG, "CONFIG", config->color);
    Debug_print(DLVL_DEBUG, "CONFIG", config->updatePath);

    return true;
}

bool SpiffsConfig_update(MokoshConfiguration config) {
    StaticJsonBuffer<300> jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();

    json["ssid"] = config.ssid;
    json["password"] = config.password;
    json["broker"] = config.broker;
    json["brokerPort"] = config.brokerPort;
    json["updateServer"] = config.updateServer;
    json["updatePort"] = config.updatePort;
    json["updatePath"] = config.updatePath;
    json["color"] = config.color;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
        Debug_print(DLVL_ERROR, "CONFIG", "Failed to write to config file");
        return false;
    }

    json.printTo(configFile);
    return true;
}

void SpiffsConfig_prettyPrint(MokoshConfiguration config) {
    StaticJsonBuffer<300> jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();

    json["ssid"] = config.ssid;
    json["password"] = config.password;
    json["broker"] = config.broker;
    json["brokerPort"] = config.brokerPort;
    json["updateServer"] = config.updateServer;
    json["updatePort"] = config.updatePort;
    json["color"] = config.color;
    json["updatePath"] = config.updatePath;

    json.prettyPrintTo(Serial);
}

void SpiffsConfig_updateField(MokoshConfiguration *config, const char *field, const char *value) {
    Debug_print(DLVL_DEBUG, "CONFIG", "Changing");
    Debug_print(DLVL_DEBUG, "CONFIG", field);
    Debug_print(DLVL_DEBUG, "CONFIG", value);

    if (strcmp(field, "ssid") == 0) {
        strcpy(config->ssid, value);
    }

    if (strcmp(field, "password") == 0) {
        strcpy(config->password, value);
    }

    if (strcmp(field, "broker") == 0) {
        strcpy(config->broker, value);
    }

    if (strcmp(field, "brokerPort") == 0) {
        config->brokerPort = String(value).toInt();
    }

    if (strcmp(field, "updateServer") == 0) {
        strcpy(config->updateServer, value);
    }

    if (strcmp(field, "updatePort") == 0) {
        config->updatePort = String(value).toInt();
    }

    if (strcmp(field, "updatePath") == 0) {
        strcpy(config->updatePath, value);
    }

    if (strcmp(field, "color") == 0) {
        String color2 = value;

        int r = color2.substring(0, 3).toInt();
        int g = color2.substring(4, 7).toInt();
        int b = color2.substring(8, 11).toInt();

        config->color = NeoPixel_convertColorToCode(r, g, b);
    }
}

void SpiffsConfig_remove() {
    SPIFFS.remove("/config.json");
}