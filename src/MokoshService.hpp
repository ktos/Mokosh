#ifndef MOKOSHSERVICE_H
#define MOKOSHSERVICE_H

#include <Arduino.h>
#include <memory>
#include <vector>

#if defined(ESP8266)
#include <Client.h>
#endif

class Mokosh;
class MokoshConfig;

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

    // marks this service is dependent on the network connection
    static const char *DEPENDENCY_NETWORK;

    // marks this service is dependent on the MQTT connection
    static const char *DEPENDENCY_MQTT;

protected:
    bool setupReady = false;
};

class MokoshNetworkService : public MokoshService
{
public:
    virtual std::shared_ptr<Client> getClient() = 0;

    virtual bool isConnected() = 0;

    virtual String getIP() = 0;

    virtual bool reconnect(std::shared_ptr<MokoshConfig> config) = 0;
};

class MokoshMqttService : public MokoshService
{
public:
    virtual bool isConnected() = 0;
    virtual bool reconnect() = 0;

    // publishes a new message on a given topic with a given payload
    virtual void publishRaw(const char *topic, const char *payload, bool retained) = 0;

    // publishes a new message on a Prefix_ABCDE/subtopic topic with
    // a given payload
    virtual void publish(const char *subtopic, String payload)
    {
        this->publish(subtopic, payload.c_str());
    }

    // publishes a new message on a Prefix_ABCDE/subtopic topic with
    // a given payload
    virtual void publish(const char *subtopic, const char *payload)
    {
        this->publish(subtopic, payload, false);
    }

    // publishes a new message on a Prefix_ABCDE/subtopic topic with
    // a given payload
    virtual void publish(const char *subtopic, const char *payload, bool retained) = 0;

    // publishes a new message on a Prefix_ABCDE/subtopic topic with
    // a given payload
    virtual void publish(const char *subtopic, float payload)
    {
        this->publish(subtopic, String(payload));
    }
};

#endif