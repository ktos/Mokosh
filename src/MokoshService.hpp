#ifndef MOKOSHSERVICE_H
#define MOKOSHSERVICE_H

#include <Arduino.h>
#include <memory>
#include <vector>

class Mokosh;

class MokoshService
{
public:
    MokoshService()
    {
    }

    virtual bool setup(std::shared_ptr<Mokosh> mokosh) = 0;
    virtual bool isSetup()
    {
        return this->setupReady;
    }

    virtual std::vector<const char *> getDependencies()
    {
        return {};
    }

    virtual void loop() = 0;

    // some built-in names for dependencies

    // this service is dependent on the network connection
    static const char *DEPENDENCY_NETWORK;

    // this service is dependent on the MQTT connection
    static const char *DEPENDENCY_MQTT;

protected:
    bool setupReady = false;
};

#endif