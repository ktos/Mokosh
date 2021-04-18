#include "Mokosh.hpp"

#if defined(ESP8266)
#include <LittleFS.h>
#endif

#if defined(ESP32)
#include <SPIFFS.h>
#define LittleFS SPIFFS
#include <ESPmDNS.h>
#endif

static Mokosh* _instance;

void _mqtt_callback(char* topic, uint8_t* message, unsigned int length) {
    _instance->mqttCommandReceived(topic, message, length);
}

void _heartbeat() {
    _instance->publish(_instance->heartbeat_topic.c_str(), millis());
}

RemoteDebug Debug;
bool Mokosh::debugReady = false;

MokoshConfiguration Mokosh::CreateConfiguration(const char* ssid, const char* password, const char* broker, uint16_t brokerPort) {
    MokoshConfiguration mc;

    strcpy(mc.ssid, ssid);
    strcpy(mc.password, password);
    strcpy(mc.broker, broker);
    mc.brokerPort = brokerPort;

    return mc;
}

Mokosh::Mokosh() {
    _instance = this;
    Mokosh::debugReady = false;

    // initialize interval events table
    for (uint8_t i = 0; i < EVENTS_COUNT; i++)
        events[i].interval = 0;
}

void Mokosh::debug(DebugLevel level, const char* func, const char* fmt, ...) {
    char dest[256];
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(dest, fmt, argptr);
    va_end(argptr);

    if (!Mokosh::isDebugReady()) {
        Serial.printf("(%s) %s [local debug]\n", func, dest);
    }

    if (Debug.isActive((uint8_t)level)) {
        Debug.printf("(%s) %s\n", func, dest);
    }
}

bool Mokosh::isDebugReady() {
    return false;
}

void Mokosh::setConfiguration(MokoshConfiguration config) {
    this->config = config;
}

void Mokosh::begin(String prefix) {
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    Serial.print("Hi, I'm ");
    Serial.print(prefix);
    Serial.println(".");

    char hostString[16] = {0};
#if defined(ESP8266)
    sprintf(hostString, "%06X", ESP.getChipId());
#endif

#if defined(ESP32)
    // strange bit manipulations to extract only 24 bits of MAC
    // like in ESP8266
    sprintf(hostString, "%06X", ((uint32_t)ESP.getEfuseMac() << 8) >> 8);
#endif

    this->prefix = prefix;
    this->hostName = String(hostString);
    strcpy(this->hostNameC, hostName.c_str());

    if (this->isFSEnabled) {
        if (!LittleFS.begin()) {
            if (!LittleFS.format()) {
                this->error(Mokosh::Error_FS);
            }
        }

        if (!this->isConfigurationSet()) {
            this->reloadConfig();
        }
    }

    if (!this->isConfigurationSet()) {
        this->error(Mokosh::Error_CONFIG);
    }

    if (this->connectWifi()) {
        Debug.begin(this->hostName, (uint8_t)this->debugLevel);
        Debug.setSerialEnabled(true);
        Debug.setResetCmdEnabled(true);
        Debug.showTime(true);

        Mokosh::debugReady = true;

        mdebugI("IP: %s", WiFi.localIP().toString().c_str());
    } else {
        if (!this->isIgnoringConnectionErrors) {
            this->error(Mokosh::Error_WIFI);
        } else {
            mdebugD("Wi-Fi connection error, ignored.");
        }
    }

    this->client = new WiFiClient();
    this->mqtt = new PubSubClient(*(this->client));

    IPAddress broker;
    broker.fromString(this->config.broker);

    this->mqtt->setServer(broker, this->config.brokerPort);
    mdebugD("MQTT broker set to %s port %d", broker.toString().c_str(), this->config.brokerPort);

    if (!this->reconnect()) {
        if (!this->isIgnoringConnectionErrors)
            this->error(Mokosh::Error_MQTT);
    }

    if (this->client->connected() && this->isOTAEnabled) {
        mdebugV("OTA is enabled. OTA port: %d", this->OTA.port);
        ArduinoOTA.setPort(this->OTA.port);
        ArduinoOTA.setHostname(this->hostNameC);

        MokoshOTAConfiguration moc = this->OTA;

        ArduinoOTA
            .onStart([moc]() {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH) {
                    type = "sketch";
                } else {
                    // U_SPIFFS
                    type = "filesystem";
                    LittleFS.end();
                }
                mdebugI("OTA started. Updating %s", type.c_str());
                if (moc.onStart != nullptr)
                    moc.onStart();
            });

        ArduinoOTA.onEnd([moc]() {
            mdebugI("OTA finished.");
            LittleFS.begin();
            if (moc.onEnd != nullptr)
                moc.onEnd();
        });

        ArduinoOTA.onProgress([moc](unsigned int progress, unsigned int total) {
            mdebugV("OTA progress: %u%%\n", (progress / (total / 100)));
            if (moc.onProgress != nullptr)
                moc.onProgress(progress, total);
        });

        ArduinoOTA.onError([moc](ota_error_t error) {
            mdebugE("OTA failed with error %u", error);
            if (error == OTA_AUTH_ERROR)
                mdebugE("Auth Failed");
            else if (error == OTA_BEGIN_ERROR)
                mdebugE("Begin Failed");
            else if (error == OTA_CONNECT_ERROR)
                mdebugE("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR)
                mdebugE("Receive Failed");
            else if (error == OTA_END_ERROR)
                mdebugE("End Failed");

            if (moc.onError != nullptr)
                moc.onError(error);
        });

        ArduinoOTA.begin();
    }

    mdebugI("Sending hello");
    this->publishShortVersion();

    mdebugD("Sending IP");
    this->publishIP();

    this->onInterval(_heartbeat, HEARTBEAT);

    mdebugI("Starting operations...");
}

void Mokosh::publishIP() {
    char msg[64] = {0};
    char ipbuf[15] = {0};
    WiFi.localIP().toString().toCharArray(ipbuf, 15);
    snprintf(msg, sizeof(msg) - 1, "{\"ipaddress\": \"%s\"}", ipbuf);

    this->publish(debug_ip_topic.c_str(), msg);
}

void Mokosh::disableFS() {
    this->isFSEnabled = false;
}

bool Mokosh::reconnect() {
    if (client->connected())
        return true;

    uint8_t trials = 0;

    if (!client->connected() && this->isIgnoringConnectionErrors) {
        mdebugV("Client not connected, but ignoring.");
        return false;
    }

    while (!client->connected()) {
        trials++;
        if (this->mqtt->connect(this->hostNameC)) {
            mdebugI("MQTT reconnected");

            char cmd_topic[32];
            sprintf(cmd_topic, "%s_%s/%s", this->prefix.c_str(), this->hostNameC, this->cmd_topic.c_str());

            this->mqtt->subscribe(cmd_topic);
            this->mqtt->setCallback(_mqtt_callback);

            return true;
        } else {
            mdebugE("MQTT failed: %d", mqtt->state());

            // if not connected in the third trial, give up
            if (trials > 3)
                return false;

            // wait 5 seconds before retrying
            delay(5000);
        }
    }

    return false;
}

PubSubClient* Mokosh::getPubSubClient() {
    return this->mqtt;
}

bool Mokosh::isConfigurationSet() {
    return strcmp(this->config.ssid, "") != 0;
}

bool Mokosh::connectWifi() {
    WiFi.enableAP(0);
    char fullHostName[32] = {0};
    sprintf(fullHostName, "%s_%s", this->prefix.c_str(), this->hostNameC);
#if defined(ESP8266)
    WiFi.hostname(fullHostName);
#else
    // workaround for https://github.com/espressif/arduino-esp32/issues/2537
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(fullHostName);
#endif
    WiFi.begin(this->config.ssid, this->config.password);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        Serial.print(".");
        delay(250);
    }

    // Check connection
    return (WiFi.status() == WL_CONNECTED);
}

Mokosh* Mokosh::getInstance() {
    return _instance;
}

bool Mokosh::configExists() {
    File configFile = LittleFS.open("/config.json", "r");

    if (!configFile) {
        return false;
    } else {
        return true;
    }
}

void Mokosh::setDebugLevel(DebugLevel level) {
    this->debugLevel = level;

    if (Mokosh::debugReady) {
        mdebugW("Setting mdebug level should be before begin(), ignoring for internals.");
    }
}

void Mokosh::loop() {
    unsigned long now = millis();

    for (uint8_t i = 0; i < EVENTS_COUNT; i++) {
        if (events[i].interval != 0) {
            if (now - events[i].last > events[i].interval) {
                mdebugV("Executing interval func %x on time %ld", (unsigned int)events[i].handler, events[i].interval);
                events[i].handler();
                events[i].last = now;
            }
        }
    }

    this->mqtt->loop();
    Debug.handle();
    ArduinoOTA.handle();
}

String Mokosh::readConfigString(const char* field) {
    const char* data = this->configJson[field];
    mdebugV("Read config.json string field %s, value %s", field, data);
    return String(data);
}

int Mokosh::readConfigInt(const char* field) {
    int data = this->configJson[field];
    mdebugV("Read config.json int field %s, value %d", field, data);
    return data;
}

float Mokosh::readConfigFloat(const char* field) {
    float data = this->configJson[field];
    mdebugV("Read config.json float field %s, value %f", field, data);
    return data;
}

void Mokosh::setConfig(const char* field, String value) {
    mdebugV("Settings config.json field %s to string %s", field, value.c_str());
    this->configJson[field] = value;
}

void Mokosh::setConfig(const char* field, int value) {
    mdebugV("Settings config.json field %s to int %d", field, value);
    this->configJson[field] = value;
}

void Mokosh::setConfig(const char* field, float value) {
    mdebugV("Settings config.json field %s to float %f", field, value);
    this->configJson[field] = value;
}

void Mokosh::saveConfig() {
    mdebugV("Saving config.json");
    File configFile = LittleFS.open("/config.json", "w");
    serializeJson(this->configJson, configFile);
}

bool Mokosh::reloadConfig() {
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

    deserializeJson(this->configJson, configFile);

    const char* ssid = this->configJson["ssid"];
    strcpy(this->config.ssid, ssid);

    const char* password = this->configJson["password"];
    strcpy(this->config.password, password);

    const char* broker = this->configJson["broker"];
    strcpy(this->config.broker, broker);

    this->config.brokerPort = this->configJson["brokerPort"];

    return true;
}

void Mokosh::factoryReset() {
    mdebugV("Removing config.json");
    LittleFS.remove("/config.json");

    ESP.restart();
}

void Mokosh::publishShortVersion() {
    int sep = this->version.indexOf('+');

    if (sep != -1) {
        String ver = this->version.substring(0, sep);
        this->publish(version_topic.c_str(), ver);
    } else {
        this->publish(version_topic.c_str(), this->version);
    }
}

void Mokosh::mqttCommandReceived(char* topic, uint8_t* message, unsigned int length) {
    if (strstr(topic, "cmd") == NULL) {
        return;
    }

    char msg[32] = {0};
    for (unsigned int i = 0; i < length; i++) {
        msg[i] = message[i];
    }
    msg[length + 1] = 0;

    mdebugD("Command: %s", msg);

    if (strcmp(msg, "gver") == 0 || strcmp(msg, "getver") == 0) {
        this->publishShortVersion();

        return;
    }

    if (strcmp(msg, "mdebug") == 0) {
        mdebugE("mdebug REQUESTED");

        return;
    }

    if (strcmp(msg, "getip") == 0) {
        this->publishIP();

        return;
    }

    if (strcmp(msg, "getfullver") == 0) {
        this->publish(debug_response_topic.c_str(), this->version);

        return;
    }

    if (strcmp(msg, "gmd5") == 0) {
        char md5[128];
        ESP.getSketchMD5().toCharArray(md5, 128);
        this->publish(debug_response_topic.c_str(), md5);

        return;
    }

    if (strcmp(msg, "getbuilddate") == 0) {
        this->publish(debug_response_topic.c_str(), this->buildDate);

        return;
    }

    if (strcmp(msg, "reboot") == 0) {
        ESP.restart();

        return;
    }

    if (strcmp(msg, "factory") == 0) {
        this->factoryReset();

        return;
    }

    if (strcmp(msg, "reloadconfig") == 0) {
        this->reloadConfig();

        return;
    }

    if (msg[0] == 's') {
        String msg2 = String(msg);

        if (msg2.startsWith("showerror=")) {
            long errorCode = msg2.substring(10).toInt();
            this->error(errorCode);
            return;
        }

        if (msg2 == "saveconfig") {
            this->saveConfig();
            return;
        }

        if (msg2.startsWith("showconfigs=")) {
            String field = msg2.substring(12);
            String value = this->readConfigString(field.c_str());

            this->publish(debug_response_topic.c_str(), value);
            return;
        }

        if (msg2.startsWith("showconfigi=")) {
            String field = msg2.substring(12);
            int value = this->readConfigInt(field.c_str());

            this->publish(debug_response_topic.c_str(), value);
            return;
        }

        if (msg2.startsWith("showconfigf=")) {
            String field = msg2.substring(12);
            float value = this->readConfigFloat(field.c_str());

            this->publish(debug_response_topic.c_str(), value);
            return;
        }

        if (msg2.startsWith("setconfigs=")) {
            String param = msg2.substring(11);

            String field = param.substring(0, param.indexOf('|'));
            String value = param.substring(param.indexOf('|') + 1);

            mdebugV("Setting configuration: field: %s, new value: %s", field.c_str(), value.c_str());

            this->setConfig(field.c_str(), value);

            return;
        }

        if (msg2.startsWith("setconfigi=")) {
            String param = msg2.substring(11);

            String field = param.substring(0, param.indexOf('|'));
            String value = param.substring(param.indexOf('|') + 1);

            mdebugV("Setting configuration: field: %s, new value: %d", field.c_str(), value.toInt());

            this->setConfig(field.c_str(), (int)(value.toInt()));

            return;
        }

        if (msg2.startsWith("setconfigf=")) {
            String param = msg2.substring(11);

            String field = param.substring(0, param.indexOf('|'));
            String value = param.substring(param.indexOf('|') + 1);

            mdebugV("Setting configuration: field: %s, new value: %f", field.c_str(), value.toFloat());

            this->setConfig(field.c_str(), value.toFloat());

            return;
        }

        if (msg2.startsWith("setdebuglevel=")) {
            long level = msg2.substring(14).toInt();
            this->setDebugLevel((DebugLevel)(int)level);
            return;
        }
    }

    if (this->commandHandler != nullptr) {
        mdebugD("Passing message to custom command handler");
        this->commandHandler(message, length);
    }
}

void Mokosh::onInterval(f_interval_t func, unsigned long time) {
    mdebugV("Registering interval function %x on time %ld", (unsigned int)func, time);

    IntervalEvent* first = NULL;
    for (uint8_t i = 0; i < EVENTS_COUNT; i++) {
        if (events[i].interval == 0) {
            first = &events[i];
            break;
        }
    }

    if (first == NULL) {
        mdebugW("Interval function cannot be registered, no free space.");
    } else {
        first->handler = func;
        first->interval = time;
    }
}

void Mokosh::onError(f_error_handler_t handler) {
    this->errorHandler = handler;
}

void Mokosh::publish(const char* subtopic, String payload) {
    this->publish(subtopic, payload.c_str());
}

void Mokosh::publish(const char* subtopic, const char* payload) {
    this->publish(subtopic, payload, false);
}

void Mokosh::publish(const char* subtopic, const char* payload, boolean retained) {
    char topic[60] = {0};
    sprintf(topic, "%s_%s/%s", this->prefix.c_str(), this->hostNameC, subtopic);

    if (!this->client->connected()) {
        this->reconnect();
    }

    this->mqtt->publish(topic, payload, retained);
}

void Mokosh::publish(const char* subtopic, float payload) {
    char spay[16];
    dtostrf(payload, 4, 2, spay);

    this->publish(subtopic, spay);
}

void Mokosh::error(int code) {
    if (!Mokosh::debugReady) {
        Serial.print("Critical error: ");
        Serial.print(code);
        Serial.println(", debug not ready.");
    }

    if (this->errorHandler != nullptr) {
        mdebugV("Handler called for error %d", code);
        this->errorHandler(code);
    } else {
        if (this->isRebootOnError) {
            mdebugE("Unhandled error, code: %d, reboot imminent.", code);
            delay(10000);
            ESP.restart();
        } else {
            mdebugE("Unhandled error, code: %d, going loop.", code);
            if (Mokosh::debugReady) {
                if (this->client->connected() && this->mqtt->state() == MQTT_CONNECTED) {
                    this->publish(this->heartbeat_topic.c_str(), "error_loop", true);
                }
            } else {
                Serial.print("Going loop.");
            }

            while (true) {
                delay(10);
            }
        }
    }
}

void Mokosh::onCommand(f_command_handler_t handler) {
    this->commandHandler = handler;
}

void Mokosh::enableRebootOnError() {
    this->isRebootOnError = true;
}

void Mokosh::setBuildMetadata(String version, String buildDate) {
    this->version = version;
    this->buildDate = buildDate;
}

void Mokosh::enableOTA() {
    this->isOTAEnabled = true;
}

void Mokosh::setIgnoreConnectionErrors(bool value) {
    this->isIgnoringConnectionErrors = value;
}