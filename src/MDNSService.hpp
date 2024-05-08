#ifndef MDNSSERVICE_H
#define MDNSSERVICE_H

#include "MokoshService.hpp"
#include <Arduino.h>
#include <memory>

namespace MokoshServices
{
    class MDNSService : public MokoshService
    {
    public:
        MDNSService() : MokoshService()
        {
        }

        virtual bool setup(std::shared_ptr<Mokosh> mokosh) override;

        // adds broadcasted MDNS service
        void addMDNSService(const char *service, const char *proto, uint16_t port);

        // adds broadcasted MDNS service props
        void addMDNSServiceProps(const char *service, const char *proto, const char *property, const char *value);
    };
}

#endif