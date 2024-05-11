#ifndef MOKOSHWIFISERVICE_H
#define MOKOSHWIFISERVICE_H

#include "MokoshService.hpp"
#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#endif

#if defined(ESP32)
#include <LittleFS.h>
#include <WiFiMulti.h>
#endif

#include <Mokosh.hpp>

class MokoshWiFiService : public MokoshNetworkService
{
public:
    MokoshWiFiService() : MokoshNetworkService()
    {
    }

    virtual bool setup() override
    {
        auto mokosh = Mokosh::getInstance();

        WiFi.enableAP(0);
        char fullHostName[32] = {0};
        sprintf(fullHostName, "%s_%s", mokosh->getPrefix().c_str(), mokosh->getHostName().c_str());
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
#endif

        this->setupFinished = true;

        return reconnect();
    }

    virtual void loop() override
    {
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
    }

    // returns "NETWORK", it's a basic network service, others are dependent
    // on it
    virtual const char *key()
    {
        return MokoshService::DEPENDENCY_NETWORK;
    }

    // event handlers for Wi-Fi situations (onConnect, onDisconnect)
    MokoshWiFiHandlers wifiEvents;

    std::shared_ptr<Client> getClient()
    {
        return client;
    }

    bool isConnected()
    {
        return WiFi.isConnected();
    }

    String getIP()
    {
        return WiFi.localIP().toString();
    }

    bool reconnect()
    {
        auto config = Mokosh::getInstance()->config;

        bool multi = config->hasKey(config->key_multi_ssid);

        if (multi)
        {
#if defined(ESP32)
            WiFiMulti wifiMulti;

            String multi = config->get<String>(config->key_multi_ssid, "");
            mlogD("Will try multiple SSID");

            StaticJsonDocument<256> doc;

            DeserializationError error = deserializeJson(doc, multi);

            if (error)
            {
                mlogE("Configured multiple ssid is wrong, deserialization error %s", error.c_str());
                return false;
            }

            for (JsonObject item : doc.as<JsonArray>())
            {
                const char *ssid = item["ssid"];
                const char *password = item["password"];

                wifiMulti.addAP(ssid, password);
            }

            if (wifiMulti.run(10000) == WL_CONNECTED)
            {
                mlogI("Connected to %s", WiFi.SSID().c_str());
            }
#else
            mlogE("Multiple SSIDs are not supported on ESP8266");
            return false;
#endif
        }
        else
        {
            String ssid = config->get<String>(config->key_ssid, "");
            String password = config->get<String>(config->key_wifi_password);

            if (ssid == "")
            {
                mlogE("Configured ssid is empty, cannot connect to Wi-Fi");
                return false;
            }

            WiFi.begin(ssid.c_str(), password.c_str());
            this->isWiFiConfigured = true;

            unsigned long startTime = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
            {
                Mokosh::debug_ticker_step();
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
            Mokosh::debug_ticker_finish(true);
        }
        else
        {
            Mokosh::debug_ticker_finish(false);
            return false;
        }

        mlogI("IP: %s", WiFi.localIP().toString().c_str());
        this->client = std::make_shared<WiFiClient>();

        return lastWifiStatus == WL_CONNECTED;
    }

private:
    bool isWiFiConfigured;
    wl_status_t lastWifiStatus;
    std::shared_ptr<WiFiClient> client = nullptr;
};

#endif