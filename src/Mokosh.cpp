#include "Mokosh.hpp"

static Mokosh *_instance;

std::vector<std::shared_ptr<DebugAdapter>> Mokosh::debugAdapters;

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
#elif defined(ESP32)
    // strange bit manipulations to extract only 24 bits of MAC
    // like in ESP8266
    uint32_t chipId = 0;
    for (int i = 0; i < 17; i = i + 8)
    {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }

    sprintf(hostString, "%06X", chipId);
#else
#error Use OVERRIDE_HOSTNAME or implement ChipID for this platform
#endif

    this->prefix = prefix;
    this->hostName = String(hostString);

#ifdef OVERRIDE_HOSTNAME
    this->hostName = OVERRIDE_HOSTNAME;
#endif

#ifndef OVERRIDE_HOSTNAME
    mdebugV("ID: %s", hostString);
#else
    mdebugV("ID: %s, overridden to %s", hostString, OVERRIDE_HOSTNAME);
#endif

    // config service is set up in the beginning, and immediately
    this->registerService(MokoshConfig::KEY, std::make_shared<MokoshConfig>(useFilesystem));
    this->config = this->getRegisteredService<MokoshConfig>(MokoshConfig::KEY);
    if (!this->config->setup())
    {
        mdebugE("Configuration Service failed!");
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

Mokosh *Mokosh::setIPRetained(bool value)
{
    this->isIPRetained = value;
    return this;
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
                this->getMqttService()->publish(_instance->heartbeat_topic, millis()); },
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
    // if there is no "NETWORK" providing service registered previously (before begin()),
    // register a Wi-Fi network providing service, if autoconnect is true,
    // as well as MQTT provider
    if (autoconnect && !this->isServiceRegistered(MokoshService::DEPENDENCY_NETWORK))
    {
        mdebugD("autoconnect, registering Wi-Fi as a network provider");
        auto network = std::make_shared<MokoshWiFiService>();
        this->registerService(MokoshService::DEPENDENCY_NETWORK, network);

        // run immediately, before all other services
        bool isConnected = MokoshResilience::Retry::retry([&network]()
                                                          { return network->setup(); }, 3, 5000, 1);

        if (!isConnected)
        {
            this->error(MokoshErrors::NetworkConnectionFailed);
        }

        if (!this->isServiceRegistered(MokoshService::DEPENDENCY_MQTT))
        {
            mdebugD("autoconnect, registering default MQTT provider");
            auto mqtt = std::make_shared<PubSubClientService>();
            this->registerService(MokoshService::DEPENDENCY_MQTT, mqtt);

            // run immediately, before all other services
            bool isConnected = MokoshResilience::Retry::retry([&mqtt]()
                                                              { return mqtt->setup(); }, 3, 5000, 1);

            if (!isConnected)
            {
                this->error(MokoshErrors::MqttConnectionFailed);
            }
        }

        this->hello();
    }

    // set up tickers and all services
    initializeTickers();
    this->setupServices();

    mdebugI("Starting operations...");
    isAfterBegin = true;
}

std::shared_ptr<MokoshMqttService> Mokosh::getMqttService()
{
    return this->getRegisteredService<MokoshMqttService>(MokoshService::DEPENDENCY_MQTT);
}

std::shared_ptr<MokoshNetworkService> Mokosh::getNetworkService()
{
    auto network = this->getRegisteredService<MokoshNetworkService>(MokoshService::DEPENDENCY_NETWORK);
    return network;
}

void Mokosh::publishIP()
{
    char msg[64] = {0};
    char ipbuf[15] = {0};

    if (this->getNetworkService() && this->getNetworkService()->isConnected())
    {
        WiFi.localIP().toString().toCharArray(ipbuf, 15);
        snprintf(msg, sizeof(msg) - 1, "{\"ipaddress\": \"%s\"}", ipbuf);

        mdebugV("Sending IP");
        this->getMqttService()->publish(debug_ip_topic, msg, this->isIPRetained);
    }
}

bool Mokosh::reconnect()
{
    auto network = this->getRegisteredService<MokoshWiFiService>(MokoshService::DEPENDENCY_NETWORK);

    if (network == nullptr)
    {
        if (this->isIgnoringConnectionErrors)
        {
            mdebugV("Network is not configured, but ignoring.");
            return true;
        }
        else
        {
            mdebugE("Network is not configured.");
            return false;
        }
    }

    if (!network->isConnected())
    {
        if (this->isIgnoringConnectionErrors)
        {
            mdebugV("Network is not connected, but ignoring.");
            return true;
        }
        else
        {
            if (this->isForceNetworkReconnect)
            {
                mdebugI("Network is not connected, forcing reconnect.");
                network->reconnect();
            }
            else
            {
                mdebugE("Network is not connected, but force-reconnect is disabled");
                return false;
            }
        }
    }

    auto mqtt = this->getMqttService();
    if (mqtt == nullptr)
    {
        if (this->isIgnoringConnectionErrors)
        {
            mdebugV("MQTT is not configured, but ignoring.");
            return true;
        }
        else
        {
            mdebugE("MQTT is not configured.");
            return false;
        }
    }

    if (network->isConnected() && !mqtt->isConnected())
    {
        if (this->isForceNetworkReconnect)
        {
            mdebugI("MQTT is not connected, forcing reconnect.");
            mqtt->reconnect();
        }
        else
        {
            mdebugE("Network is not connected, but force-reconnect is disabled");
            return false;
        }
    }

    return network->isConnected() && mqtt->isConnected();
}

Mokosh *Mokosh::setForceWiFiReconnect(bool value)
{
    this->isForceNetworkReconnect = value;
    return this;
}

Mokosh *Mokosh::setHeartbeat(bool value)
{
    this->isHeartbeatEnabled = value;
    return this;
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

    this->reconnect();

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
        this->getMqttService()->publish(version_topic, ver);
        mdebugD("Version: %s", ver.c_str());
    }
    else
    {
        this->getMqttService()->publish(version_topic, this->version);
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
        this->getMqttService()->publish(debug_response_topic, this->version);

        return;
    }

    if (command == "gmd5")
    {
        char md5[128];
        ESP.getSketchMD5().toCharArray(md5, 128);
        mdebugV("Firmware MD5: %s", md5);
        this->getMqttService()->publish(debug_response_topic, md5);

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
            // if (isWifiConnected())
            {
                // if (this->client->connected() && this->mqtt->state() == MQTT_CONNECTED)
                // {
                //     this->publish(this->heartbeat_topic, "error_loop", true);
                // }
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
        return this->registerDebugAdapter(String(random(1, 100), HEX).c_str(), service);
    }
    else
    {
        return this->registerDebugAdapter(key, service);
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
    auto network = this->getNetworkService();
    auto mqtt = this->getRegisteredService<MokoshWiFiService>(MokoshService::DEPENDENCY_MQTT);

    if (isDependentOn(service, MokoshService::DEPENDENCY_NETWORK) && (network == nullptr))
    {
        mdebugE("Network dependent service %s cannot be set up because Network is not configured", key);
        return this;
    }

    if (isDependentOn(service, MokoshService::DEPENDENCY_MQTT) && (network == nullptr || mqtt == nullptr))
    {
        mdebugE("MQTT dependent service %s cannot be set up because Network or MQTT are not configured", key);
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
        service->setup();

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