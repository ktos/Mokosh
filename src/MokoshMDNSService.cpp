#include "MokoshMDNSService.hpp"
#include "Mokosh.hpp"

#if defined(ESP8266)
#include <ESP8266mDNS.h>
#endif

#if defined(ESP32)
#include <ESPmDNS.h>
#endif

namespace MokoshServices
{
    const char *MDNSService::KEY = "MDNS";

    bool MDNSService::setup()
    {
        auto mokosh = Mokosh::getInstance();
        if (!MDNS.begin(mokosh->getHostNameWithPrefix()))
        {
            mdebugE("MDNS couldn't be enabled");
            return false;
        }

        this->addMDNSService("mokosh", "tcp", 23);
        this->addMDNSServiceProps("mokosh", "tcp", "version", mokosh->getVersion().c_str());

        this->setupReady = true;

        return true;
    }

    const char *key()
    {
        return MDNSService::KEY;
    }

    void MDNSService::addMDNSService(const char *service, const char *proto, uint16_t port)
    {
        MDNS.addService(service, proto, port);
    }

    void MDNSService::addMDNSServiceProps(const char *service, const char *proto, const char *property, const char *value)
    {
        MDNS.addServiceTxt(service, proto, property, value);
    }

    void MDNSService::loop()
    {
    }
}