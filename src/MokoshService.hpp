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

typedef std::function<void(String, uint8_t *msg, unsigned int length)> THandlerFunction_Message;

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
    virtual bool setup() = 0;

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

    virtual bool reconnect() = 0;
};

class MokoshMqttService : public MokoshService
{
public:
    // returns if the Mqtt Service is connected to the network
    virtual bool isConnected() = 0;

    // reconnects the MQTT service with the network
    virtual bool reconnect() = 0;

    // returns if this is a first connection attempt or subsequent
    virtual bool isFirstConnection() = 0;

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

    // subscribes to a given topic
    virtual void subscribe(const char *topic) = 0;

    // unsubscribes from a given topic
    virtual void unsubscribe(const char *topic) = 0;

    // defines callback to be run when message is received
    // (e.g. in MQTT to a subscribed topic)
    THandlerFunction_Message onMessage;
};

#endif