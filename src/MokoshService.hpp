#ifndef MOKOSHSERVICE_H
#define MOKOSHSERVICE_H

#include <Arduino.h>
#include <memory>
#include <vector>

class Mokosh;

// a class representing a Mokosh Service, a class which is started
// with dependency on another classes and is being looped along with others
class MokoshService
{
public:
    MokoshService()
    {
    }

    // sets up a new instance of a service, an instance to the main class
    // is passed to check some data or read configuration
    virtual bool setup(std::shared_ptr<Mokosh> mokosh) = 0;

    // returns if the class has been set up properly
    virtual bool isSetup()
    {
        return this->setupReady;
    }

    // returns a list of other services this service is dependent on
    // may include some built-in like NET or MQTT which have consts
    // below
    virtual std::vector<const char *> getDependencies()
    {
        return {};
    }

    // handles a command
    virtual bool command(String command, String param)
    {
        return false;
    }

    // loop, run internally by Mokosh:loop()
    virtual void loop() = 0;

    // returns default name for keyed registration of the service
    virtual const char *key()
    {
        return "";
    }

    // some built-in names for dependencies

    // this service is dependent on the network connection
    static const char *DEPENDENCY_NETWORK;

    // this service is dependent on the MQTT connection
    static const char *DEPENDENCY_MQTT;

protected:
    bool setupReady = false;
};

#endif