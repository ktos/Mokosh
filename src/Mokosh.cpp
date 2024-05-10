#include "Mokosh.hpp"

static Mokosh *_instance;

std::vector<std::shared_ptr<DebugAdapter>> Mokosh::debugAdapters;

void _mqtt_callback(char *topic, uint8_t *message, unsigned int length)
{
    _instance->_mqttCommandReceived(topic, message, length);
}

Mokosh::Mokosh(String prefix, String version, bool useFilesystem, bool useSerial)
{
    _instance = this;

    this->version = version;

    if (useSerial)
    {
        Serial.begin(115200);
        Serial.println();
        Serial.println();
        Serial.print("Hi, I'm ");
        Serial.print(prefix);
        Serial.println(".");

        this->registerDebugAdapter("SERIAL_DEBUG", std::make_shared<SerialDebugAdapter>());
    }

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

#ifndef OVERRIDE_HOSTNAME
    mdebugV("ID: %s", hostString);
#else
    mdebugV("ID: %s, overridden to %s", hostString, OVERRIDE_HOSTNAME);
#endif

    // config service is set up in the beginning, and immediately
    this->registerService(MokoshConfig::KEY, std::make_shared<MokoshConfig>(useFilesystem));
    this->config = this->getRegisteredService<MokoshConfig>(MokoshConfig::KEY);
    if (!this->config->setup(std::make_shared<Mokosh>(*this)))
    {
        mdebugE("Configuration system error!");
    }
}

void Mokosh::debug(LogLevel level, const char *func, const char *file, int line, const char *fmt, ...)
{
    char dest[256];
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(dest, fmt, argptr);
    va_end(argptr);

    for (auto &adapter : Mokosh::debugAdapters)
    {
        if (adapter->isActive(level))
        {
            adapter->debugf(level, func, file, line, millis(), dest);
        }
    }
}

void Mokosh::debug_ticker_step()
{
    for (auto &adapter : Mokosh::debugAdapters)
    {
        adapter->ticker_step();
    }
}

void Mokosh::debug_ticker_finish(bool success)
{
    for (auto &adapter : Mokosh::debugAdapters)
    {
        adapter->ticker_finish(success);
    }
}

bool Mokosh::configureMqttClient()
{
    IPAddress broker;
    String brokerAddress = this->config->get<String>(this->config->key_broker);
    if (brokerAddress == "")
    {
        mdebugE("MQTT configuration is not provided!");
        return false;
    }
    uint16_t brokerPort = this->config->get<int>(this->config->key_broker_port, 1883);

    // it it is an IP address
    if (broker.fromString(brokerAddress))
    {
        this->mqtt->setServer(broker, brokerPort);
        mdebugV("MQTT broker set to %s port %d", broker.toString().c_str(), brokerPort);
    }
    else
    {
        // so it must be a domain name
        this->mqtt->setServer(brokerAddress.c_str(), brokerPort);
        mdebugV("MQTT broker set to %s port %d", brokerAddress.c_str(), brokerPort);
    }

    this->isMqttConfigured = true;
    return true;
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

Mokosh *Mokosh::setIPRetained(bool value)
{
    this->isIPRetained = value;
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

String Mokosh::getVersion()
{
    return this->version;
}

void Mokosh::begin(bool autoconnect)
{
    if (autoconnect)
    {
        this->connectWifi();
        if (this->lastWifiStatus == WL_CONNECTED)
        {
            // this->setupRemoteDebug();
            this->setupWiFiClient();
            this->setupMqttClient();

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
                mdebugI("Wi-Fi connection error but ignored.");
            }
        }
    }
    else
    {
        mdebugI("Auto network connection was disabled!");
    }

    initializeTickers();
    this->setupServices();

    mdebugI("Starting operations...");
    isAfterBegin = true;
}

void Mokosh::publishIP()
{
    char msg[64] = {0};
    char ipbuf[15] = {0};

    if (this->isWifiConnected())
    {
        WiFi.localIP().toString().toCharArray(ipbuf, 15);
        snprintf(msg, sizeof(msg) - 1, "{\"ipaddress\": \"%s\"}", ipbuf);

        mdebugD("Sending IP");
        this->publish(debug_ip_topic, msg, this->isIPRetained);
    }
}

bool Mokosh::reconnect()
{
    if (this->mqtt->connected())
        return true;

    if (this->isWifiConfigured)
    {
        if (!this->isWifiConnected() && this->isForceWifiReconnect)
        {
            mdebugD("Wi-Fi is not connected at all, forcing reconnect.");
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
        mdebugD("Wi-Fi is not configured, raising onReconnectRequest event.");
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
        if (this->config->hasKey(this->config->key_client_id))
            clientId = this->config->get<String>(this->config->key_client_id, this->hostName);

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
                mdebugD("Client not connected, but ignoring.");
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
    // workaround for https://github.com/espressif/arduino-esp32/issues/6700#issuecomment-1140331981
    WiFi.config(((u32_t)0x0UL), ((u32_t)0x0UL), ((u32_t)0x0UL));
    WiFi.mode(WIFI_MODE_NULL);
    WiFi.setHostname(fullHostName);

    bool multi = this->config->hasKey(this->config->key_multi_ssid);
#else
    bool multi = false;
#endif
    if (multi)
    {
#if defined(ESP32)
        WiFiMulti wifiMulti;

        String multi = this->config->get<String>(this->config->key_multi_ssid, "");
        mdebugD("Will try multiple SSID");

        StaticJsonDocument<256> doc;

        DeserializationError error = deserializeJson(doc, multi);

        if (error)
        {
            mdebugE("Configured multiple ssid is wrong, deserialization error %s", error.c_str());
            return wl_status_t::WL_NO_SSID_AVAIL;
        }

        for (JsonObject item : doc.as<JsonArray>())
        {
            const char *ssid = item["ssid"];
            const char *password = item["password"];

            wifiMulti.addAP(ssid, password);
        }

        if (wifiMulti.run(10000) == WL_CONNECTED)
        {
            mdebugI("Connected to %s", WiFi.SSID().c_str());
        }
#endif
    }
    else
    {
        String ssid = this->config->get<String>(this->config->key_ssid, "");
        String password = this->config->get<String>(this->config->key_wifi_password);

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
            debug_ticker_step();
            delay(250);
        }
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
        debug_ticker_finish(true);
    }
    else
    {
        debug_ticker_finish(false);
    }

    return lastWifiStatus;
}

Mokosh *Mokosh::getInstance()
{
    return _instance;
}

Mokosh *Mokosh::setLogLevel(LogLevel level)
{
    this->debugLevel = level;

    for (auto &debug : this->debugAdapters)
    {
        debug->setActive(level);
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

    if (!this->mqtt->connected())
    {
        this->reconnect();
    }

    if (this->mqtt != nullptr)
        this->mqtt->loop();

    for (auto &service : this->services)
    {
        service.second->loop();
    }
}

void Mokosh::factoryReset()
{
    mdebugI("Factory reset initialized");
    LittleFS.format();
    ESP.restart();
}

void Mokosh::publishShortVersion()
{
    int sep = this->version.indexOf('+');

    if (sep != -1)
    {
        String ver = this->version.substring(0, sep);
        this->publish(version_topic, ver);
        mdebugD("Version: %s", ver.c_str());
    }
    else
    {
        this->publish(version_topic, this->version);
        mdebugD("Version: %s", this->version.c_str());
    }
}

void Mokosh::_processCommand(String command)
{
    String param = "";
    int eq = command.indexOf('=');
    if (eq > -1)
    {
        param = command.substring(eq + 1);
        command = command.substring(0, eq);
    }
    mdebugI("Command: %s, param %s", command.c_str(), param.c_str());

    // try the built-in commands first
    if (command == "gver" || command == "getver")
    {
        mdebugV("Version: %s", this->version.c_str());
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
        mdebugV("Version: %s", this->version.c_str());
        this->publish(debug_response_topic, this->version);

        return;
    }

    if (command == "gmd5")
    {
        char md5[128];
        ESP.getSketchMD5().toCharArray(md5, 128);
        mdebugV("Firmware MD5: %s", md5);
        this->publish(debug_response_topic, md5);

        return;
    }

    if (command == "reboot")
    {
        ESP.restart();

        return;
    }

    if (command == "factory")
    {
        this->factoryReset();
        return;
    }

    if (command == "showerror")
    {
        long errorCode = param.toInt();
        mdebugE("Error initiated: %ld", errorCode);
        this->error(errorCode);
        return;
    }

    if (command == "setdebuglevel")
    {
        long level = param.toInt();
        this->setLogLevel((LogLevel)(int)level);
        return;
    }

    // now try passing to the services
    for (auto &service : this->services)
    {
        if (service.second->command(command, param))
            return;
    }

    // if they were not handled, pass to the custom command handler
    if (this->onCommand != nullptr)
    {
        mdebugD("Passing command to custom command handler");
        this->onCommand(command, param);
    }
}

void Mokosh::_mqttCommandReceived(char *topic, uint8_t *message, unsigned int length)
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
        mdebugD("MQTT command: %s", msg);
        String command = String(msg);
        this->_processCommand(command);
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
    mdebugD("Registering interval function on time %ld", time);
    std::shared_ptr<TickTwo> ticker = std::make_shared<TickTwo>(func, time, 0, MILLIS);
    this->tickers.push_back(ticker);

    if (this->isAfterBegin)
    {
        mdebugD("Called after begin(), running ticker immediately");
        ticker->start();
        return;
    }
}

void Mokosh::registerTimeoutFunction(fptr func, unsigned long time, int runs, bool start)
{
    mdebugD("Registering oneshot function on time %ld that will run %d times", time, runs);
    std::shared_ptr<TickTwo> ticker = std::make_shared<TickTwo>(func, time, runs, MILLIS);
    this->tickers.push_back(ticker);

    if (start)
    {
        ticker->start();
        return;
    }
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
    if (this->debugAdapters.size() == 0)
    {
        Serial.begin(115200);
        Serial.print("Critical error: ");
        Serial.print(code);
        Serial.println(", no debug adapters.");
    }

    if (this->onError != nullptr)
    {
        mdebugD("Handler called for error %d", code);
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
            if (isWifiConnected())
            {
                if (this->client->connected() && this->mqtt->state() == MQTT_CONNECTED)
                {
                    this->publish(this->heartbeat_topic, "error_loop", true);
                }
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

Mokosh *Mokosh::setIgnoreConnectionErrors(bool value)
{
    this->isIgnoringConnectionErrors = value;
    return this;
}

String Mokosh::getPrefix()
{
    return this->prefix;
}

String Mokosh::getHostName()
{
    return this->hostName;
}

String Mokosh::getHostNameWithPrefix()
{
    String result = this->prefix + "_" + this->hostName;
    return result;
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

bool isDependentOn(std::shared_ptr<MokoshService> service, const char *key)
{
    auto it = std::find(service->getDependencies().begin(), service->getDependencies().end(), key);
    return (it != service->getDependencies().end());
}

Mokosh *Mokosh::registerService(std::shared_ptr<MokoshService> service)
{
    const char *key = service->key();
    if (strcmp(key, "") == 0)
    {
        return this->registerService(String(random(1, 100), HEX).c_str(), service);
    }
    else
    {
        return this->registerService(key, service);
    }
}

Mokosh *Mokosh::registerService(const char *key, std::shared_ptr<MokoshService> service)
{
    this->services[key] = service;

    if (this->isAfterBegin)
    {
        mdebugI("Service %s registered after begin, setting up immediately", key);
        setupService(key, service);
    }

    return this;
}

Mokosh *Mokosh::registerDebugAdapter(std::shared_ptr<DebugAdapter> service)
{
    const char *key = service->key();
    if (strcmp(key, "") == 0)
    {
        return this->registerService(String(random(1, 100), HEX).c_str(), service);
    }
    else
    {
        return this->registerService(key, service);
    }
}

Mokosh *Mokosh::registerDebugAdapter(const char *key, std::shared_ptr<DebugAdapter> service)
{
    // adding both to the services list as well as special list of only debug adapters
    this->services[key] = service;

    if (this->isAfterBegin)
    {
        mdebugI("Debug Adapter %s registered after begin, setting up immediately", key);
        setupService(key, service);
    }

    Mokosh::debugAdapters.push_back(service);

    return this;
}

bool Mokosh::setupService(const char *key, std::shared_ptr<MokoshService> service)
{
    if (isDependentOn(service, MokoshService::DEPENDENCY_NETWORK) && (!isWifiConfigured))
    {
        mdebugE("Wi-Fi dependent service %s cannot be set up because Wi-Fi is not configured", key);
        return this;
    }

    if (isDependentOn(service, MokoshService::DEPENDENCY_MQTT) && (!isWifiConfigured || !isMqttConfigured))
    {
        mdebugE("MQTT dependent service %s cannot be set up because Wi-Fi or MQTT are not configured", key);
        return this;
    }

    // here should be some kind of dependency graph resolution
    bool depsReady = true;
    for (auto &deps : service->getDependencies())
    {
        // built-in network and MQTT services are ignored
        if (strcmp(deps, MokoshService::DEPENDENCY_NETWORK) == 0)
            continue;

        if (strcmp(deps, MokoshService::DEPENDENCY_MQTT) == 0)
            continue;

        if (this->services.find(deps) == this->services.end())
        {
            mdebugE("Cannot setup service %s because the dependency %s is not set up yet.", key, deps);
            return false;
            break;
        }
    }

    // services are not being setup more than once
    if (!service->isSetup())
        service->setup(std::make_shared<Mokosh>(*this));

    return true;
}

void Mokosh::setupServices()
{
    for (auto &service : this->services)
    {
        auto key = service.first;
        auto value = service.second;

        setupService(key, value);
    }
}

bool Mokosh::isServiceRegistered(const char *key)
{
    return this->services.find(key) != this->services.end();
}