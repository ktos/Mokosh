#ifndef OTASERVICE_H
#define OTASERVICE_H

#include "MokoshService.hpp"
#include <Arduino.h>
#include <memory>
#include <Mokosh.hpp>

namespace MokoshServices
{
    class OTAService : public MokoshService
    {
    public:
        OTAService() : MokoshService()
        {
        }

        virtual bool setup(std::shared_ptr<Mokosh> mokosh) override
        {
            uint16_t otaPort = 3232;
#if defined(ESP8266)
            otaPort = this->config.get<int>(this->config.key_ota_port, 8266);
#endif

#if defined(ESP32)
            otaPort = mokosh->config.get<int>(mokosh->config.key_ota_port, 3232);
#endif

            mdebugV("OTA is enabled. OTA port: %d", otaPort);
            ArduinoOTA.setPort(otaPort);
            ArduinoOTA.setHostname(mokosh->getHostName().c_str());

            String hash = mokosh->config.get<String>(mokosh->config.key_ota_password);
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

        virtual std::vector<const char *> getDependencies()
        {
            return {MokoshService::DEPENDENCY_NETWORK};
        }

        virtual void loop() override
        {
            ArduinoOTA.handle();
        }

        // event handlers for OTA situations (onStart, onEnd, etc.)
        MokoshOTAHandlers otaEvents;

        virtual const char *getDefaultKey()
        {
            return "OTA";
        }

    private:
        bool isOTAInProgress = false;
    };
}

#endif