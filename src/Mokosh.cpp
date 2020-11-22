#include "Mokosh.hpp"

#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <LittleFS.h>
#include <RemoteDebug.h>

static Mokosh* _instance;

void _mqtt_callback(char* topic, uint8_t* message, unsigned int length) {
    _instance->mqttCommandReceived(topic, message, length);
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

    if (this->isConfigurationSet()) {
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
    debugV("MQTT server set to %s port %d", broker.toString().c_str(), this->config.brokerPort);

    if (!this->reconnect()) {
        this->error(Mokosh::Error_MQTT);
    }

    debugV("Sending hello");
    this->publish(this->version_topic.c_str(), VERSION);

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
            sprintf(cmd_topic, "%s_%s/cmd", this->prefix.c_str(), this->hostNameC, this->cmd_topic.c_str());

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
    return strcmp(this->config.ssid, "") == 0;
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
}

void Mokosh::loop() {
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

    // TODO: parsing config file
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

    debugV("Command: %s", msg);

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
        // TODO: reload config from FS

        return;
    }

    if (msg[0] == 's') {
        String msg2 = String(msg);

        if (msg2.startsWith("showerror=")) {
            long errorCode = msg2.substring(10).toInt();
            this->error(errorCode);
            return;
        }
    }

    if (this->commandHandler != nullptr) {
        debugV("Passing message to custom command handler");
        this->commandHandler(message, length);
    }
}

void Mokosh::onInterval(f_interval_t func, uint32_t time) {
    // dodaj do jakiegoś wektora
    // i w loopie sprawdzaj czas
    // i odpalaj funkcje zgodnie z harmonogramem
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
    if (this->debugReady) {
        debugE("Critical error: %d", code);
    } else {
        Serial.print("Critical error: ");
        Serial.print(code);
        Serial.println(", debug not ready.");
    }

    if (this->errorHandler != nullptr) {
        this->errorHandler(code);
    } else {
        if (this->isRebootOnError) {
            delay(10000);
            ESP.reset();
        } else {
            if (this->debugReady) {
                debugD("Going loop.");
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