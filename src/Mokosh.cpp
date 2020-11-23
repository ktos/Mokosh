#include "Mokosh.hpp"

#include <ArduinoJson.h>
#include <ESP8266httpUpdate.h>
#include <LittleFS.h>

static Mokosh* _instance;

void _mqtt_callback(char* topic, uint8_t* message, unsigned int length) {
    _instance->mqttCommandReceived(topic, message, length);
}

void _heartbeat() {
    _instance->publish(_instance->heartbeat_topic.c_str(), millis());
}

RemoteDebug Debug;

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
    this->debugReady = false;

    // initialize interval events table
    for (uint8_t i = 0; i < EVENTS_COUNT; i++)
        events[i].interval = 0;
}

void Mokosh::debug(DebugLevel level, const char* func, const char* fmt, ...) {
    if (Debug.isActive((uint8_t)level)) {
        char dest[256];
        va_list argptr;
        va_start(argptr, fmt);
        vsprintf(dest, fmt, argptr);
        va_end(argptr);

        Debug.printf("(%s) %s\n", func, dest);
    }
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
    sprintf(hostString, "%06X", ESP.getChipId());

    this->prefix = prefix;
    this->hostName = String(hostString);
    strcpy(this->hostNameC, hostName.c_str());

    if (this->isFSEnabled) {
        if (!LittleFS.begin()) {
            this->error(Mokosh::Error_FS);
        }

        if (!this->isConfigurationSet()) {
            this->configLoad();
        }
    }

    if (!this->isConfigurationSet()) {
        this->error(Mokosh::Error_CONFIG);
    }

    if (this->connectWifi()) {
        Debug.begin(this->hostName, (uint8_t)this->debugLevel);
        Debug.setSerialEnabled(true);

        this->debugReady = true;

        debugI("IP: %s", WiFi.localIP().toString().c_str());
    } else {
        this->error(Mokosh::Error_WIFI);
    }

    this->client = new WiFiClient();
    this->mqtt = new PubSubClient(*(this->client));

    IPAddress broker;
    broker.fromString(this->config.broker);

    this->mqtt->setServer(broker, this->config.brokerPort);
    debugD("MQTT broker set to %s port %d", broker.toString().c_str(), this->config.brokerPort);

    if (!this->reconnect()) {
        this->error(Mokosh::Error_MQTT);
    }

    debugI("Sending hello");
    this->publish(this->version_topic.c_str(), VERSION);

    this->onInterval(_heartbeat, HEARTBEAT);

    debugI("Starting operations...");
}

void Mokosh::disableFS() {
    this->isFSEnabled = false;
}

bool Mokosh::reconnect() {
    uint8_t trials = 0;

    while (!client->connected()) {
        trials++;
        if (this->mqtt->connect(this->hostNameC)) {
            debugI("MQTT reconnected");

            char cmd_topic[32];
            sprintf(cmd_topic, "%s_%s/%s", this->prefix.c_str(), this->hostNameC, this->cmd_topic.c_str());

            this->mqtt->subscribe(cmd_topic);
            this->mqtt->setCallback(_mqtt_callback);

            return true;
        } else {
            debugE("MQTT failed: %d", mqtt->state());

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
    WiFi.hostname(this->hostNameC);
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

    if (this->debugReady) {
        debugW("Setting debug level should be before begin(), ignoring for internals.");
    }
}

void Mokosh::loop() {
    unsigned long now = millis();

    for (uint8_t i = 0; i < EVENTS_COUNT; i++) {
        if (events[i].interval != 0) {
            if (now - events[i].last > events[i].interval) {
                debugV("Executing interval func %x on time %ld", (unsigned int)events[i].handler, events[i].interval);
                events[i].handler();
                events[i].last = now;
            }
        }
    }

    this->mqtt->loop();
    Debug.handle();
}

bool Mokosh::configLoad() {
    File configFile = LittleFS.open("/config.json", "r");

    if (!configFile) {
        debugE("Cannot open config.json file");
        return false;
    }

    size_t size = configFile.size();
    if (size > 1024) {
        debugE("Config file too large");
        return false;
    }

    DynamicJsonDocument conf(1024);
    deserializeJson(conf, configFile);

    const char* ssid = conf["ssid"];
    strcpy(this->config.ssid, ssid);

    const char* password = conf["password"];
    strcpy(this->config.password, password);

    const char* broker = conf["broker"];
    strcpy(this->config.broker, broker);

    this->config.brokerPort = conf["brokerPort"];

    return true;
}

void Mokosh::factoryReset() {
    LittleFS.remove("/config.json");

    ESP.restart();
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

    debugD("Command: %s", msg);

    if (strcmp(msg, "gver") == 0) {
        this->publish(version_topic.c_str(), VERSION);

        return;
    }

    if (strcmp(msg, "getfullver") == 0) {
        this->publish(debug_topic.c_str(), INFORMATIONAL_VERSION);

        return;
    }

    if (strcmp(msg, "gmd5") == 0) {
        char md5[128];
        ESP.getSketchMD5().toCharArray(md5, 128);
        this->publish(debug_topic.c_str(), md5);

        return;
    }

    if (strcmp(msg, "getbuilddate") == 0) {
        this->publish(debug_topic.c_str(), BUILD_DATE);

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
        this->configLoad();

        return;
    }

    if (msg[0] == 's') {
        String msg2 = String(msg);

        if (msg2.startsWith("showerror=")) {
            long errorCode = msg2.substring(10).toInt();
            this->error(errorCode);
            return;
        }

        if (msg2.startsWith("setdebuglevel=")) {
            long level = msg2.substring(14).toInt();
            this->setDebugLevel((DebugLevel)(int)level);
            return;
        }
    }

    if (this->commandHandler != nullptr) {
        debugD("Passing message to custom command handler");
        this->commandHandler(message, length);
    }
}

void Mokosh::onInterval(f_interval_t func, unsigned long time) {
    debugV("Registering interval function %x on time %ld", (unsigned int)func, time);

    IntervalEvent* first = NULL;
    for (uint8_t i = 0; i < EVENTS_COUNT; i++) {
        if (events[i].interval == 0) {
            first = &events[i];
            break;
        }
    }

    if (first == NULL) {
        debugW("Interval function cannot be registered, no free space.");
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
    char topic[60] = {0};
    sprintf(topic, "%s_%s/%s", this->prefix.c_str(), this->hostNameC, subtopic);

    this->mqtt->publish(topic, payload);
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

    if (this->errorHandler != nullptr) {
        debugV("Handler called for error %d", code);
        this->errorHandler(code);
    } else {
        if (this->isRebootOnError) {
            debugE("Unhandled error, code: %d, reboot imminent.", code);
            delay(10000);
            ESP.reset();
        } else {
            debugE("Unhandled error, code: %d", code);
            if (this->debugReady) {
                debugV("Going loop.");
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