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

MokoshConfiguration create_configuration(const char* ssid, const char* password, const char* broker, uint16_t brokerPort, const char* updateServer, uint16_t updatePort, const char* updatePath) {
    MokoshConfiguration mc;

    strcpy(mc.ssid, ssid);
    strcpy(mc.password, password);
    strcpy(mc.broker, broker);
    mc.brokerPort = brokerPort;
    strcpy(mc.updateServer, updateServer);
    mc.updatePort = updatePort;
    strcpy(mc.updatePath, updatePath);

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

        if (this->isConfigurationSet()) {
            if (!this->configLoad()) {
                if (this->isFirstRunEnabled) {
                    // first run
                    // TODO: first run mode
                    this->error(Mokosh::Error_NOTIMPLEMENTED);
                } else {
                    this->error(Mokosh::Error_CONFIG);
                }
            }
        }
    }

    if (this->isConfigurationSet()) {
        this->error(Mokosh::Error_CONFIG);
    }

    if (this->connectWifi()) {
        Debug.begin(this->hostName, (uint8_t)this->debugLevel);
        Debug.setSerialEnabled(true);
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

// bool Mokosh::configLoad(MokoshConfiguration* config) {
//     File configFile = LittleFS.open("/config.json", "r");

//     if (!configFile) {
//         Debug_print(DLVL_ERROR, "CONFIG", "Cannot open config file");
//         return false;
//     }

//     size_t size = configFile.size();
//     if (size > 1024) {
//         Debug_print(DLVL_ERROR, "CONFIG", "Config file too large");
//         return false;
//     }

//     // Allocate a buffer to store contents of the file.
//     std::unique_ptr<char[]> buf(new char[size]);

//     // We don't use String here because ArduinoJson library requires the input
//     // buffer to be mutable. If you don't use ArduinoJson, you may as well
//     // use configFile.readString instead.
//     configFile.readBytes(buf.get(), size);

//     StaticJsonBuffer<300> jsonBuffer;
//     JsonObject& json = jsonBuffer.parseObject(buf.get());

//     if (!json.success()) {
//         Debug_print(DLVL_ERROR, "CONFIG", "Failed to parse config file");
//         return false;
//     }

//     const char* ssid2 = json["ssid"];
//     const char* password2 = json["password"];
//     const char* broker2 = json["broker"];
//     const char* update2 = json["updateServer"];

//     memcpy(config->ssid, ssid2, 32);
//     memcpy(config->password, password2, 32);
//     memcpy(config->broker, broker2, 32);
//     memcpy(config->updateServer, update2, 32);

//     config->brokerPort = json["brokerPort"];
//     config->updatePort = json["updatePort"];

//     const char* color = json["color"];
//     String color2 = color;

//     if (color2.length() == 11) {
//         int r = color2.substring(0, 3).toInt();
//         int g = color2.substring(4, 7).toInt();
//         int b = color2.substring(8, 11).toInt();

//         config->color = NeoPixel_convertColorToCode(r, g, b);
//     } else {
//         config->color = (uint32_t)json["color"];
//     }

//     Debug_print(DLVL_DEBUG, "CONFIG", config->ssid);
//     Debug_print(DLVL_DEBUG, "CONFIG", config->password);
//     Debug_print(DLVL_DEBUG, "CONFIG", config->broker);
//     Debug_print(DLVL_DEBUG, "CONFIG", config->brokerPort);
//     Debug_print(DLVL_DEBUG, "CONFIG", config->color);
//     Debug_print(DLVL_DEBUG, "CONFIG", config->updatePath);

//     return true;
// }

// bool Mokosh::configUpdate(MokoshConfiguration config) {
//     StaticJsonBuffer<300> jsonBuffer;
//     JsonObject& json = jsonBuffer.createObject();

//     json["ssid"] = config.ssid;
//     json["password"] = config.password;
//     json["broker"] = config.broker;
//     json["brokerPort"] = config.brokerPort;
//     json["updateServer"] = config.updateServer;
//     json["updatePort"] = config.updatePort;
//     json["updatePath"] = config.updatePath;
//     json["color"] = config.color;

//     File configFile = LittleFS.open("/config.json", "w");
//     if (!configFile) {
//         Debug_print(DLVL_ERROR, "CONFIG", "Failed to write to config file");
//         return false;
//     }

//     json.printTo(configFile);
//     return true;
// }

// void Mokosh::configprettyPrint(MokoshConfiguration config) {
//     StaticJsonBuffer<300> jsonBuffer;
//     JsonObject& json = jsonBuffer.createObject();

//     json["ssid"] = config.ssid;
//     json["password"] = config.password;
//     json["broker"] = config.broker;
//     json["brokerPort"] = config.brokerPort;
//     json["updateServer"] = config.updateServer;
//     json["updatePort"] = config.updatePort;
//     json["color"] = config.color;
//     json["updatePath"] = config.updatePath;

//     json.prettyPrintTo(Serial);
// }

// void Mokosh::configUpdateField(MokoshConfiguration* config, const char* field, const char* value) {
//     Debug_print(DLVL_DEBUG, "CONFIG", "Changing");
//     Debug_print(DLVL_DEBUG, "CONFIG", field);
//     Debug_print(DLVL_DEBUG, "CONFIG", value);

//     if (strcmp(field, "ssid") == 0) {
//         strcpy(config->ssid, value);
//     }

//     if (strcmp(field, "password") == 0) {
//         strcpy(config->password, value);
//     }

//     if (strcmp(field, "broker") == 0) {
//         strcpy(config->broker, value);
//     }

//     if (strcmp(field, "brokerPort") == 0) {
//         config->brokerPort = String(value).toInt();
//     }

//     if (strcmp(field, "updateServer") == 0) {
//         strcpy(config->updateServer, value);
//     }

//     if (strcmp(field, "updatePort") == 0) {
//         config->updatePort = String(value).toInt();
//     }

//     if (strcmp(field, "updatePath") == 0) {
//         strcpy(config->updatePath, value);
//     }

//     if (strcmp(field, "color") == 0) {
//         String color2 = value;

//         int r = color2.substring(0, 3).toInt();
//         int g = color2.substring(4, 7).toInt();
//         int b = color2.substring(8, 11).toInt();

//         config->color = NeoPixel_convertColorToCode(r, g, b);
//     }
// }

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
        // Debug_print(DLVL_INFO, "CONFIG", "Reloading configuration from SPIFFS");
        // SpiffsConfig_load(&config);

        return;
    }

    if (msg[0] == 's') {
        String msg2 = String(msg);

        if (msg2.startsWith("showerror=")) {
            long errorCode = msg2.substring(10).toInt();
            this->error(errorCode);
            return;
        }

        if (msg2.startsWith("sota=")) {
            if (!this->isOtaEnabled)
                return;

            String version = msg2.substring(5);

            char buf[32];
            version.toCharArray(buf, 32);

            this->handleOta(buf);
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

void Mokosh::handleOta(char* version) {
    char uri[128];
    sprintf(uri, this->config.updatePath, version);

    debugI("Starting OTA update to version %s", version);

    t_httpUpdate_return ret = ESPhttpUpdate.update(this->config.updateServer, this->config.updatePort, uri);

    if (ret == HTTP_UPDATE_FAILED) {
        debugW("OTA update failed");
    }
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

/*
ESP8266WebServer server(80);



void FirstRun_configure(const char *_version, const char *_informationalVersion, const char *_buildDate) {
    strcpy(version, _version);
    strcpy(informationalVersion, _informationalVersion);
    strcpy(buildDate, _buildDate);
}

void handleRoot() {
    server.send(200, "text/plain", "Hi, I'm Mokosh.");
}

File fsUploadFile;

void handleFileUpload() {
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        fsUploadFile = SPIFFS.open("/config.json", "w");

        Debug_print(DLVL_DEBUG, "FIRSTRUN-CONFIG", "Upload started");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (fsUploadFile)
            fsUploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (fsUploadFile)
            fsUploadFile.close();

        Debug_print(DLVL_DEBUG, "FIRSTRUN-CONFIG", "Upload finished");
        Debug_print(DLVL_DEBUG, "FIRSTRUN-CONFIG", (int)upload.totalSize);
    }
}

void handleWhois() {
    String whois = "{ ";
    whois += "\"id\": \"";
    whois += ssid;
    whois += "\",\n";

    whois += "\"version\": \"";
    whois += version;
    whois += "\",\n";

#if DEBUG_LEVEL < 4
    whois += "\"informational_version\": \"";
    whois += informationalVersion;
    whois += "\",\n";
#endif

#if DEBUG_LEVEL < 4
    whois += "\"build_date\": \"";
    whois += buildDate;
    whois += "\",\n";
#endif

    whois += "\"md5\": \"";
    whois += ESP.getSketchMD5();
    whois += "\"\n";

    whois += " }";

    server.send(200, "application/json", whois);
}

void handleNotFound() {
    String message = "File Not Found\n\n";

#if DEBUG_LEVEL < 4
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
#endif

    server.send(404, "text/plain", message);
}

void handleReboot() {
    server.send(200, "text/plain", "Good bye.");

    delay(1000);
    server.close();
    server.stop();
    WiFi.disconnect(true);
    delay(1000);

    ESP.restart();
}

void FirstRun_start(const char *prefix) {
    sprintf(ssid, "%s_%06X", prefix, ESP.getChipId());

    WiFi.softAP(ssid);

    IPAddress myIP = WiFi.softAPIP();

    Debug_print(DLVL_INFO, "FIRSTRUN", "Started First Run");
    Debug_print(DLVL_INFO, "FIRSTRUN", myIP.toString());

    server.onNotFound(handleNotFound);
    server.on("/", handleRoot);
    server.on("/whois", handleWhois);
    server.on("/reboot", HTTPMethod::HTTP_POST, handleReboot);
    server.on(
        "/config", HTTPMethod::HTTP_POST, []() { server.send(200, "text/plain", ""); }, handleFileUpload);

    server.begin();
}

void FirstRun_handle() {
    server.handleClient();
}
*/