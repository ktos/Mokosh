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

RemoteDebug Debug;

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

    if (!_instance->isDebugReady()) {
        char lvl;
        switch (level) {
            case DebugLevel::DEBUG:
                lvl = 'D';
                break;
            case DebugLevel::ERROR:
                lvl = 'E';
                break;
            case DebugLevel::INFO:
                lvl = 'I';
                break;
            case DebugLevel::WARNING:
                lvl = 'W';
                break;
            case DebugLevel::VERBOSE:
                lvl = 'V';
                break;
            default:
                lvl = 'A';
        }

        Serial.printf("L (%c t:%ldms) (%s) %s\n", lvl, millis(), func, dest);
    }

    if (Debug.isActive((uint8_t)level)) {
        Debug.printf("(%s) %s\n", func, dest);
    }
}

bool Mokosh::isDebugReady() {
    return this->debugReady;
}

bool Mokosh::configureMqttClient() {
    IPAddress broker;
    String brokerAddress = this->readConfigString(Mokosh::broker_field.c_str());
    if (brokerAddress == "") {
        mdebugE("MQTT configuration is not provided!");
        return false;
    }

    broker.fromString(brokerAddress);
    uint16_t brokerPort = this->readConfigInt(Mokosh::broker_port_field.c_str());

    this->mqtt->setServer(broker, brokerPort);
    mdebugD("MQTT broker set to %s port %d", broker.toString().c_str(), brokerPort);
    return true;
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

    lastWifiStatus = WiFi.status();

    this->prefix = prefix;
    this->hostName = String(hostString);
    strcpy(this->hostNameC, hostName.c_str());

    if (this->isFSEnabled) {
        if (!LittleFS.begin()) {
            if (!LittleFS.format()) {
                this->error(Mokosh::Error_FS);
            }
        }

        this->reloadConfig();
    }

    if (!this->isConfigurationSet()) {
        this->error(Mokosh::Error_CONFIG);
    }

    if (this->connectWifi()) {
        Debug.begin(this->hostName, (uint8_t)this->debugLevel);
        Debug.setSerialEnabled(true);
        Debug.setResetCmdEnabled(true);
        Debug.showTime(true);

        this->debugReady = true;

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

    if (this->configureMqttClient()) {
        if (!this->reconnect()) {
            if (!this->isIgnoringConnectionErrors)
                this->error(Mokosh::Error_MQTT);
        }
    }

    if (this->client->connected() && this->isOTAEnabled) {
        uint16_t otaPort = this->readConfigInt(Mokosh::ota_port_field.c_str(), 3232);
        mdebugV("OTA is enabled. OTA port: %d", otaPort);
        ArduinoOTA.setPort(otaPort);
        ArduinoOTA.setHostname(this->hostNameC);

        String hash = this->readConfigString(Mokosh::ota_password_field.c_str());
        if (hash != "")
            ArduinoOTA.setPasswordHash(hash.c_str());

        MokoshOTAHandlers moc = this->OtaHandlers;

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

    mdebugV("Sending IP");
    this->publishIP();

    this->onInterval([&]() {
        publish(_instance->heartbeat_topic.c_str(), millis());
    },
                     HEARTBEAT, "HEARTBEAT");

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
    if (this->mqtt->connected())
        return true;

    if (!this->isWifiConnected() && this->isForceWifiReconnect) {
        mdebugV("Wi-Fi is not connected at all, forcing reconnect.");
        bool result = this->connectWifi();
        if (!result) {
            if (this->isIgnoringConnectionErrors) {
                mdebugE("Failed to reconnect to Wi-Fi");
            } else {
                this->error(Mokosh::Error_WIFI);
            }
        }
    }

    uint8_t trials = 0;

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
            if (this->isIgnoringConnectionErrors) {
                mdebugV("Client not connected, but ignoring.");
                return false;
            }

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
    return this->readConfigString("ssid") != "";
}

bool Mokosh::isWifiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void Mokosh::setForceWiFiReconnect(bool value) {
    this->isForceWifiReconnect = value;
}

bool Mokosh::connectWifi() {
    WiFi.enableAP(0);
    char fullHostName[32] = {0};
    sprintf(fullHostName, "%s_%s", this->prefix.c_str(), this->hostNameC);
#if defined(ESP8266)
    WiFi.hostname(fullHostName);
#endif

#if defined(ESP32)
    // workaround for https://github.com/espressif/arduino-esp32/issues/2537
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(fullHostName);
#endif

    String ssid = readConfigString(ssid_field.c_str(), "");
    String password = readConfigString(wifi_password_field.c_str());

    if (ssid == "") {
        mdebugE("Configured ssid is empty, cannot connect to Wi-Fi");
        return false;
    }

    WiFi.begin(ssid, password);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        Serial.print(".");
        delay(250);
    }

    wl_status_t wifiStatus = WiFi.status();
    if (wifiStatus != lastWifiStatus && wifiStatus == WL_CONNECTED) {
        if (this->WiFiHandlers.onConnect != nullptr)
            this->WiFiHandlers.onConnect();
    } else if (wifiStatus != lastWifiStatus && wifiStatus == WL_CONNECT_FAILED) {
        if (this->WiFiHandlers.onConnectFail != nullptr)
            this->WiFiHandlers.onConnectFail();
    }
    lastWifiStatus = wifiStatus;

    bool status = lastWifiStatus == WL_CONNECTED;

    if (status == true) {
        Serial.println(" ok");
    } else {
        Serial.println(" fail");
    }

    return status;
}

Mokosh* Mokosh::getInstance() {
    return _instance;
}

bool Mokosh::configFileExists() {
    File configFile = LittleFS.open("/config.json", "r");

    if (!configFile) {
        return false;
    } else {
        return true;
    }
}

void Mokosh::setDebugLevel(DebugLevel level) {
    this->debugLevel = level;

    if (this->debugReady) {
        mdebugW("Setting mdebug level should be before begin(), ignoring for internals.");
    }
}

void Mokosh::loop() {
    unsigned long now = millis();

    for (uint8_t i = 0; i < EVENTS_COUNT; i++) {
        if (events[i].interval != 0) {
            if (now - events[i].last > events[i].interval) {
                mdebugV("Executing interval func %s on time %ld", events[i].name.c_str(), events[i].interval);
                events[i].handler();
                events[i].last = now;
            }
        }
    }

    wl_status_t wifiStatus = WiFi.status();
    if (wifiStatus != lastWifiStatus) {
        lastWifiStatus = wifiStatus;

        if (lastWifiStatus == WL_DISCONNECTED) {
            if (this->WiFiHandlers.onDisconnect != nullptr)
                this->WiFiHandlers.onDisconnect();
        }
    }

    this->mqtt->loop();
    Debug.handle();
    ArduinoOTA.handle();
}

String Mokosh::readConfigString(const char* field, String def) {
    if (!this->config.containsKey(field)) {
        mdebugW("config.json field %s does not exist!", field);
        return def;
    }

    const char* data = this->config[field];
    return String(data);
}

int Mokosh::readConfigInt(const char* field, int def) {
    if (!this->config.containsKey(field)) {
        mdebugW("config.json field %s does not exist!", field);
        return def;
    }

    int data = this->config[field];
    return data;
}

float Mokosh::readConfigFloat(const char* field, float def) {
    if (!this->config.containsKey(field)) {
        mdebugW("config.json field %s does not exist!", field);
        return def;
    }

    float data = this->config[field];
    return data;
}

void Mokosh::setConfig(const char* field, String value) {
    this->config[field] = value;
}

void Mokosh::setConfig(const char* field, int value) {
    this->config[field] = value;
}

void Mokosh::setConfig(const char* field, float value) {
    this->config[field] = value;
}

void Mokosh::saveConfig() {
    mdebugV("Saving config.json");
    File configFile = LittleFS.open("/config.json", "w");
    serializeJson(this->config, configFile);
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

    deserializeJson(this->config, configFile);

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
        mdebugV("Version: %s", ver.c_str());
    } else {
        this->publish(version_topic.c_str(), this->version);
        mdebugV("Version: %s", this->version.c_str());
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

    if (this->onCommand != nullptr) {
        mdebugD("Passing message to custom command handler");
        this->onCommand(message, length);
    }
}

void Mokosh::onInterval(THandlerFunction func, unsigned long time, String name) {
    mdebugV("Registering interval function %s on time %ld", name.c_str(), time);

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
        first->name = name;
    }
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

    if (!this->client->remoteIP().isSet()) {
        mdebugW("Cannot publish, broker address is not configured");
        return;
    }

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
    if (!this->debugReady) {
        Serial.print("Critical error: ");
        Serial.print(code);
        Serial.println(", debug not ready.");
    }

    if (this->onError != nullptr) {
        mdebugV("Handler called for error %d", code);
        this->onError(code);
    } else {
        if (this->isRebootOnError) {
            mdebugE("Unhandled error, code: %d, reboot imminent.", code);
            delay(10000);
            ESP.restart();
        } else {
            mdebugE("Unhandled error, code: %d, going loop.", code);
            if (this->debugReady) {
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