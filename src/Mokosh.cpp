#include "Mokosh.hpp"

static Mokosh *_instance;

void _mqtt_callback(char *topic, uint8_t *message, unsigned int length)
{
    _instance->mqttCommandReceived(topic, message, length);
}

RemoteDebug Debug;

Mokosh::Mokosh()
{
    _instance = this;
    Mokosh::debugReady = false;
}

void Mokosh::debug(DebugLevel level, const char *func, const char *file, int line, const char *fmt, ...)
{
    char dest[256];
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(dest, fmt, argptr);
    va_end(argptr);

    if (!_instance->isDebugReady())
    {
        char lvl;
        switch (level)
        {
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

        Serial.printf("L (%c t:%ldms) (%s %s:%d) %s\n", lvl, millis(), func, file, line, dest);
    }

    if (Debug.isActive((uint8_t)level))
    {
        Debug.printf("(%s %s:%d) %s\n", func, file, line, dest);
    }
}

bool Mokosh::isDebugReady()
{
    return this->debugReady;
}

bool Mokosh::configureMqttClient()
{
    IPAddress broker;
    String brokerAddress = this->config.get<String>(this->config.key_broker);
    if (brokerAddress == "")
    {
        mdebugE("MQTT configuration is not provided!");
        return false;
    }
    uint16_t brokerPort = this->config.get<int>(this->config.key_broker_port, 1883);

    // it it is an IP address
    if (broker.fromString(brokerAddress))
    {
        this->mqtt->setServer(broker, brokerPort);
        mdebugD("MQTT broker set to %s port %d", broker.toString().c_str(), brokerPort);
    }
    else
    {
        // so it must be a domain name
        this->mqtt->setServer(brokerAddress.c_str(), brokerPort);
        mdebugD("MQTT broker set to %s port %d", brokerAddress.c_str(), brokerPort);
    }

    this->isMqttConfigured = true;
    return true;
}

void handleRemoteDebugCommand()
{
    String cmd = Debug.getLastCommand();
    _instance->processCommand(cmd);
}

void Mokosh::setupRemoteDebug()
{
    Debug.begin(this->hostName, (uint8_t)this->debugLevel);
    Debug.setSerialEnabled(true);
    Debug.setResetCmdEnabled(true);
    Debug.showTime(true);
    Debug.setCallBackProjectCmds(handleRemoteDebugCommand);

    this->debugReady = true;
}

void Mokosh::setupWiFiClient()
{
    mdebugI("IP: %s", WiFi.localIP().toString().c_str());

    this->client = new WiFiClient();
}

Mokosh *Mokosh::setCustomClient(Client &client)
{
    this->client = &client;
    return this;
}

void Mokosh::setupMqttClient()
{
    this->mqtt = new PubSubClient(*(this->client));

    if (this->configureMqttClient())
    {
        if (!this->reconnect())
        {
            if (!this->isIgnoringConnectionErrors)
                this->error(MokoshErrors::MqttConnectionFailed);
        }
    }
}

void Mokosh::setupMDNS()
{
    if (!MDNS.begin(this->hostNameC))
    {
        mdebugE("MDNS couldn't be enabled");
        return;
    }

    this->addMDNSService("mokosh", "tcp", 23);
    this->addMDNSServiceProps("mokosh", "tcp", "version", this->version.c_str());
}

void Mokosh::setupOta()
{
    uint16_t otaPort = 3232;
#if defined(ESP8266)
    otaPort = this->config.get<int>(this->config.key_ota_port, 8266);
#endif

#if defined(ESP32)
    otaPort = this->config.get<int>(this->config.key_ota_port, 3232);
#endif

    mdebugV("OTA is enabled. OTA port: %d", otaPort);
    ArduinoOTA.setPort(otaPort);
    ArduinoOTA.setHostname(this->hostNameC);

    String hash = this->config.get<String>(this->config.key_ota_password);
    if (hash != "")
        ArduinoOTA.setPasswordHash(hash.c_str());

    MokoshOTAHandlers moc = this->otaEvents;

    ArduinoOTA
        .onStart([&]()
                 {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH) {
                type = "sketch";
            } else {
                // U_SPIFFS
                type = "filesystem";
                LittleFS.end();
            }

            this->isOTAInProgress = true;

            mdebugI("OTA started. Updating %s", type.c_str());
            if (moc.onStart != nullptr)
                moc.onStart(); });

    ArduinoOTA.onEnd([&]()
                     {
        mdebugI("OTA finished.");
        LittleFS.begin();
        this->isOTAInProgress = false;

        if (moc.onEnd != nullptr)
            moc.onEnd(); });

    ArduinoOTA.onProgress([moc](unsigned int progress, unsigned int total)
                          {
        mdebugV("OTA progress: %u%%\n", (progress / (total / 100)));
        if (moc.onProgress != nullptr)
            moc.onProgress(progress, total); });

    ArduinoOTA.onError([moc](ota_error_t error)
                       {
        String err;
        if (error == OTA_AUTH_ERROR)
            err = "auth error";
        else if (error == OTA_BEGIN_ERROR)
            err = "begin failed";
        else if (error == OTA_CONNECT_ERROR)
            err = "connect failed";
        else if (error == OTA_RECEIVE_ERROR)
            err = "receive failed";
        else if (error == OTA_END_ERROR)
            err = "end failed";

        mdebugE("OTA failed with error %u (%s)", error, err.c_str());

        if (moc.onError != nullptr)
            moc.onError(error); });

    ArduinoOTA.begin();
}

std::vector<std::shared_ptr<TickTwo>> Mokosh::getTickers()
{
    return tickers;
}

void Mokosh::hello()
{
    mdebugI("Sending hello");
    this->publishShortVersion();
    this->publishIP();

    if (this->isHeartbeatEnabled)
    {
        this->registerIntervalFunction([&]()
                                       {
            if (this->isHeartbeatEnabled)
                publish(_instance->heartbeat_topic, millis()); },
                                       HEARTBEAT);
    }
}

void Mokosh::initializeTickers()
{
    // starting all tickers
    for (auto &ticker : tickers)
    {
        ticker->start();
    }
}

void Mokosh::begin(String prefix, bool autoconnect)
{
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
    uint32_t chipId = 0;
    for (int i = 0; i < 17; i = i + 8)
    {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }

    sprintf(hostString, "%06X", chipId);
#endif

    lastWifiStatus = WiFi.status();

    this->prefix = prefix;
    this->hostName = String(hostString);

#ifdef OVERRIDE_HOSTNAME
    this->hostName = OVERRIDE_HOSTNAME;
#endif

    strcpy(this->hostNameC, hostName.c_str());
    mdebugD("ID: %s", hostString);

    if (this->isFSEnabled)
    {
        if (!this->config.prepareFS())
        {
            this->error(MokoshErrors::FileSystemNotAvailable);
        }

        this->config.reload();
    }

    if (autoconnect)
    {
        this->connectWifi();
        if (this->lastWifiStatus == WL_CONNECTED)
        {
            this->setupRemoteDebug();
            this->setupWiFiClient();
            this->setupMqttClient();

            if (this->isMDNSEnabled)
            {
                this->setupMDNS();
            }

            if (this->isOTAEnabled)
            {
                this->setupOta();
            }

            this->hello();
            this->isWifiConfigured = true;
        }
        else
        {
            if (!this->isIgnoringConnectionErrors)
            {
                this->error(MokoshErrors::CannotConnectToWifi);
            }
            else
            {
                mdebugD("Wi-Fi connection error, ignored.");
            }
        }
    }
    else
    {
        mdebugI("Auto network connection was disabled!");
    }

    initializeTickers();

    mdebugI("Starting operations...");
}

void Mokosh::publishIP()
{
    char msg[64] = {0};
    char ipbuf[15] = {0};

    if (this->isWifiConnected())
    {
        WiFi.localIP().toString().toCharArray(ipbuf, 15);
        snprintf(msg, sizeof(msg) - 1, "{\"ipaddress\": \"%s\"}", ipbuf);

        mdebugV("Sending IP");
        this->publish(debug_ip_topic, msg, true);
    }
}

Mokosh *Mokosh::setConfigFile(bool value)
{
    this->isFSEnabled = value;
    return this;
}

bool Mokosh::reconnect()
{
    if (this->mqtt->connected())
        return true;

    if (this->isWifiConfigured)
    {
        if (!this->isWifiConnected() && this->isForceWifiReconnect)
        {
            mdebugV("Wi-Fi is not connected at all, forcing reconnect.");
            bool result = this->connectWifi();
            if (!result)
            {
                if (this->isIgnoringConnectionErrors)
                {
                    mdebugE("Failed to reconnect to Wi-Fi");
                }
                else
                {
                    this->error(MokoshErrors::CannotConnectToWifi);
                }
            }
        }
    }
    else
    {
        mdebugV("Wi-Fi is not configured, raising onReconnectRequest event.");
        if (this->events.onReconnectRequested != nullptr)
        {
            this->events.onReconnectRequested();
        }
    }

    uint8_t trials = 0;

    while (!client->connected())
    {
        trials++;

        String clientId = this->hostName;
        if (this->config.hasKey(this->config.key_client_id))
            clientId = this->config.get<String>(this->config.key_client_id, this->hostName);

        if (this->mqtt->connect(clientId.c_str()))
        {
            mdebugI("MQTT reconnected");

            char cmd_topic[32];
            sprintf(cmd_topic, "%s_%s/%s", this->prefix.c_str(), this->hostNameC, this->cmd_topic);

            this->mqtt->subscribe(cmd_topic);
            this->mqtt->setCallback(_mqtt_callback);

            return true;
        }
        else
        {
            if (this->isIgnoringConnectionErrors)
            {
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

PubSubClient *Mokosh::getPubSubClient()
{
    return this->mqtt;
}

bool Mokosh::isWifiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

Mokosh *Mokosh::setForceWiFiReconnect(bool value)
{
    this->isForceWifiReconnect = value;
    return this;
}

Mokosh *Mokosh::setHeartbeat(bool value)
{
    this->isHeartbeatEnabled = value;
    return this;
}

wl_status_t Mokosh::connectWifi()
{
    WiFi.enableAP(0);
    char fullHostName[32] = {0};
    sprintf(fullHostName, "%s_%s", this->prefix.c_str(), this->hostNameC);
#if defined(ESP8266)
    WiFi.hostname(fullHostName);
#endif

#if defined(ESP32)
    // workaround for https://github.com/espressif/arduino-esp32/issues/2537
    // workaround for https://github.com/espressif/arduino-esp32/issues/4732
    WiFi.config(((u32_t)0x0UL), ((u32_t)0x0UL), ((u32_t)0x0UL));
    WiFi.setHostname(fullHostName);
#endif

    String ssid = this->config.get<String>(this->config.key_ssid, "");
    String password = this->config.get<String>(this->config.key_wifi_password);

    if (ssid == "")
    {
        mdebugE("Configured ssid is empty, cannot connect to Wi-Fi");
        return wl_status_t::WL_NO_SSID_AVAIL;
    }

    WiFi.begin(ssid.c_str(), password.c_str());
    this->isWifiConfigured = true;

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
    {
        Serial.print(".");
        delay(250);
    }

    wl_status_t wifiStatus = WiFi.status();
    if (wifiStatus != lastWifiStatus && wifiStatus == WL_CONNECTED)
    {
        if (this->wifiEvents.onConnect != nullptr)
            this->wifiEvents.onConnect();
    }
    else if (wifiStatus != lastWifiStatus && wifiStatus == WL_CONNECT_FAILED)
    {
        if (this->wifiEvents.onConnectFail != nullptr)
            this->wifiEvents.onConnectFail();
    }
    lastWifiStatus = wifiStatus;

    if (lastWifiStatus == WL_CONNECTED)
    {
        Serial.println(" ok");
    }
    else
    {
        Serial.println(" fail");
    }

    return lastWifiStatus;
}

Mokosh *Mokosh::getInstance()
{
    return _instance;
}

Mokosh *Mokosh::setDebugLevel(DebugLevel level)
{
    this->debugLevel = level;

    if (this->debugReady)
    {
        mdebugW("Setting mdebug level should be before begin(), ignoring for internals.");
    }

    return this;
}

void Mokosh::loop()
{
    unsigned long now = millis();

    // updating all registered tickers
    for (auto &ticker : tickers)
    {
        ticker->update();
    }

    wl_status_t wifiStatus = WiFi.status();
    if (wifiStatus != lastWifiStatus)
    {
        lastWifiStatus = wifiStatus;

        if (lastWifiStatus == WL_DISCONNECTED)
        {
            if (this->wifiEvents.onDisconnect != nullptr)
                this->wifiEvents.onDisconnect();
        }
    }

    if (this->mqtt != nullptr)
        this->mqtt->loop();

    Debug.handle();
    ArduinoOTA.handle();
}

void Mokosh::factoryReset()
{
    this->config.removeFile();
    ESP.restart();
}

void Mokosh::publishShortVersion()
{
    int sep = this->version.indexOf('+');

    if (sep != -1)
    {
        String ver = this->version.substring(0, sep);
        this->publish(version_topic, ver);
        mdebugV("Version: %s", ver.c_str());
    }
    else
    {
        this->publish(version_topic, this->version);
        mdebugV("Version: %s", this->version.c_str());
    }
}

void Mokosh::processCommand(String command)
{
    if (command == "gver" || command == "getver")
    {
        mdebugD("Version: %s", this->version.c_str());
        this->publishShortVersion();

        return;
    }

    if (command == "mdebug")
    {
        mdebugE("mdebug REQUESTED");

        return;
    }

    if (command == "getip")
    {
        this->publishIP();

        return;
    }

    if (command == "getfullver")
    {
        mdebugD("Version: %s", this->version.c_str());
        this->publish(debug_response_topic, this->version);

        return;
    }

    if (command == "gmd5")
    {
        char md5[128];
        ESP.getSketchMD5().toCharArray(md5, 128);
        mdebugD("Firmware MD5: %s", md5);
        this->publish(debug_response_topic, md5);

        return;
    }

    if (command == "getbuilddate")
    {
        mdebugD("Build date: %s", this->buildDate.c_str());
        this->publish(debug_response_topic, this->buildDate);

        return;
    }

    if (command == "reboot")
    {
        ESP.restart();

        return;
    }

    if (command == "factory")
    {
        mdebugW("Factory reset initiated");
        this->factoryReset();

        return;
    }

    if (command == "reloadconfig")
    {
        mdebugI("Config reload initiated");
        this->config.reload();

        return;
    }

    if (command.startsWith("showerror="))
    {
        long errorCode = command.substring(10).toInt();
        mdebugE("Error initiated: %ld", errorCode);
        this->error(errorCode);
        return;
    }

    if (command == "saveconfig")
    {
        mdebugD("Config saved");
        this->config.save();
        return;
    }

    if (command.startsWith("showconfigs="))
    {
        String field = command.substring(12);
        String value = this->config.get<String>(field.c_str());

        this->publish(debug_response_topic, value);
        mdebugD("config %s = %s", field.c_str(), value.c_str());
        return;
    }

    if (command.startsWith("showconfigi="))
    {
        String field = command.substring(12);
        int value = this->config.get<int>(field.c_str());

        this->publish(debug_response_topic, value);
        mdebugD("config %s = %i", field.c_str(), value);
        return;
    }

    if (command.startsWith("showconfigf="))
    {
        String field = command.substring(12);
        float value = this->config.get<float>(field.c_str());

        this->publish(debug_response_topic, value);
        mdebugD("config %s = %f", field.c_str(), value);
        return;
    }

    if (command.startsWith("setconfigs="))
    {
        String param = command.substring(11);

        String field = param.substring(0, param.indexOf('|'));
        String value = param.substring(param.indexOf('|') + 1);

        mdebugD("Setting configuration: field: %s, new value: %s", field.c_str(), value.c_str());

        this->config.set(field.c_str(), value);

        return;
    }

    if (command.startsWith("setconfigi="))
    {
        String param = command.substring(11);

        String field = param.substring(0, param.indexOf('|'));
        String value = param.substring(param.indexOf('|') + 1);

        mdebugD("Setting configuration: field: %s, new value: %d", field.c_str(), value.toInt());

        this->config.set(field.c_str(), (int)(value.toInt()));

        return;
    }

    if (command.startsWith("setconfigf="))
    {
        String param = command.substring(11);

        String field = param.substring(0, param.indexOf('|'));
        String value = param.substring(param.indexOf('|') + 1);

        mdebugD("Setting configuration: field: %s, new value: %f", field.c_str(), value.toFloat());

        this->config.set(field.c_str(), value.toFloat());

        return;
    }

    if (command.startsWith("setdebuglevel="))
    {
        long level = command.substring(14).toInt();
        this->setDebugLevel((DebugLevel)(int)level);
        return;
    }

    if (this->onCommand != nullptr)
    {
        mdebugV("Passing command to custom command handler");
        this->onCommand(command);
    }
}

void Mokosh::mqttCommandReceived(char *topic, uint8_t *message, unsigned int length)
{
    if (length > 64)
    {
        mdebugE("MQTT message too long, ignoring.");
        return;
    }

    char msg[64] = {0};
    for (unsigned int i = 0; i < length; i++)
    {
        msg[i] = message[i];
    }
    msg[length + 1] = 0;

    if (String(topic) == this->getMqttPrefix() + String(this->cmd_topic))
    {
        mdebugV("MQTT command: %s", msg);
        String command = String(msg);
        this->processCommand(command);
    }
    else
    {
        if (this->onMessage != nullptr)
        {
            this->onMessage(String(topic), message, length);
        }
        else
        {
            mdebugW("MQTT message received, but no handler, ignoring.");
        }
    }
}

void Mokosh::registerIntervalFunction(fptr func, unsigned long time)
{
    mdebugV("Registering interval function on time %ld", time);
    std::shared_ptr<TickTwo> ticker = std::make_shared<TickTwo>(func, time, 0, MILLIS);
    this->tickers.push_back(ticker);
}

void Mokosh::publish(const char *subtopic, String payload)
{
    this->publish(subtopic, payload.c_str());
}

void Mokosh::publish(const char *subtopic, const char *payload)
{
    this->publish(subtopic, payload, false);
}

void Mokosh::publish(const char *subtopic, const char *payload, boolean retained)
{
    char topic[60] = {0};
    sprintf(topic, "%s_%s/%s", this->prefix.c_str(), this->hostNameC, subtopic);

    if (this->client == nullptr)
    {
        mdebugE("Cannot publish, Client was not constructed!");
        return;
    }

    if (this->mqtt == nullptr)
    {
        mdebugE("Cannot publish, MQTT Client was not constructed!");
        return;
    }

    if (!this->isMqttConfigured)
    {
        mdebugE("Cannot publish, broker address is not configured");
        return;
    }

    if (!this->client->connected())
    {
        this->reconnect();
    }

    this->mqtt->publish(topic, payload, retained);
}

void Mokosh::publish(const char *subtopic, float payload)
{
    char spay[16];
    dtostrf(payload, 4, 2, spay);

    this->publish(subtopic, spay);
}

void Mokosh::error(int code)
{
    if (!this->debugReady)
    {
        Serial.print("Critical error: ");
        Serial.print(code);
        Serial.println(", debug not ready.");
    }

    if (this->onError != nullptr)
    {
        mdebugV("Handler called for error %d", code);
        this->onError(code);
    }
    else
    {
        if (this->isRebootOnError)
        {
            mdebugE("Unhandled error, code: %d, reboot imminent.", code);
            delay(10000);
            ESP.restart();
        }
        else
        {
            mdebugE("Unhandled error, code: %d, going loop.", code);
            if (this->debugReady)
            {
                if (this->client->connected() && this->mqtt->state() == MQTT_CONNECTED)
                {
                    this->publish(this->heartbeat_topic, "error_loop", true);
                }
            }
            else
            {
                Serial.print("Going loop.");
            }

            while (true)
            {
                delay(10);
            }
        }
    }
}

Mokosh *Mokosh::setRebootOnError(bool value)
{
    this->isRebootOnError = value;
    return this;
}

Mokosh *Mokosh::setBuildMetadata(String version, String buildDate)
{
    this->version = version;
    this->buildDate = buildDate;
    return this;
}

Mokosh *Mokosh::setOta(bool value)
{
    this->isOTAEnabled = value;

    if (this->isOTAEnabled)
        this->isMDNSEnabled = true;

    return this;
}

Mokosh *Mokosh::setIgnoreConnectionErrors(bool value)
{
    this->isIgnoringConnectionErrors = value;
    return this;
}

Mokosh *Mokosh::setMDNS(bool value)
{
    this->isMDNSEnabled = value;
    return this;
}

void Mokosh::addMDNSService(const char *service, const char *proto, uint16_t port)
{
    MDNS.addService(service, proto, port);
}

void Mokosh::addMDNSServiceProps(const char *service, const char *proto, const char *property, const char *value)
{
    MDNS.addServiceTxt(service, proto, property, value);
}

String Mokosh::getPrefix()
{
    return this->prefix;
}

String Mokosh::getHostName()
{
    return this->hostName;
}

String Mokosh::getMqttPrefix()
{
    String result = this->prefix + "_" + this->hostName + "/";
    return result;
}

Client *Mokosh::getClient()
{
    return this->client;
}