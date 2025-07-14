#ifndef PUBSUBCLIENTSERVICE_H
#define PUBSUBCLIENTSERVICE_H

#if defined(ESP32) || defined(ESP8266)

#include "MokoshService.hpp"
#include <PubSubClient.h>
#include <Mokosh.hpp>

class PubSubClientService : public MokoshMqttService
{
public:
    PubSubClientService() : MokoshMqttService()
    {
    }

    virtual bool setup() override
    {
        auto mokosh = Mokosh::getInstance();
        this->network = mokosh->getNetworkService();
        this->mqtt = std::make_shared<PubSubClient>(*mokosh->getNetworkService()->getClient());

        IPAddress broker;
        String brokerAddress = mokosh->config->get<String>(mokosh->config->key_broker);
        if (brokerAddress == "")
        {
            mlogE("MQTT configuration is not provided!");
            return false;
        }
        uint16_t brokerPort = mokosh->config->get<int>(mokosh->config->key_broker_port, 1883);

        // it it is an IP address
        if (broker.fromString(brokerAddress))
        {
            this->mqtt->setServer(broker, brokerPort);
            mlogI("MQTT broker set to %s port %d", broker.toString().c_str(), brokerPort);
        }
        else
        {
            // so it must be a domain name
            this->mqtt->setServer(brokerAddress.c_str(), brokerPort);
            mlogI("MQTT broker set to %s port %d", brokerAddress.c_str(), brokerPort);
        }

        this->isMqttConfigured = true;
        this->mqttPrefix = mokosh->getMqttPrefix();

        this->clientId = mokosh->getHostNameWithPrefix();
        if (mokosh->config->hasKey(mokosh->config->key_client_id))
            this->clientId = mokosh->config->get<String>(mokosh->config->key_client_id, mokosh->getHostNameWithPrefix());

        this->setupFinished = true;
        this->cmd_topic = mokosh->getMqttPrefix() + String(mokosh->cmd_topic);
        this->reconnectCount = 0;

        return reconnect();
    }

    virtual bool isFirstConnection()
    {
        return this->reconnectCount == 0;
    }

    virtual void loop() override
    {
        this->mqtt->loop();
    }

    // returns "MQTT", it's a quite basic network service, others are dependent
    // on it
    virtual const char *key()
    {
        return MokoshService::DEPENDENCY_MQTT;
    }

    // this service is dependent on network connection
    virtual std::vector<const char *> getDependencies()
    {
        return {MokoshService::DEPENDENCY_NETWORK};
    }

    virtual bool isConnected()
    {
        return this->mqtt->connected();
    }

    virtual bool reconnect()
    {
        // TODO: crashes when the Wi-Fi connection is restored

        if (this->mqtt->connect(clientId.c_str()))
        {
            mlogI("MQTT reconnected");

            this->mqtt->setCallback([&](char *topic, uint8_t *message, unsigned int length)
                                    { this->_mqttCommandReceived(topic, message, length); });

            // resubscribing to all subscribed topics (and cmd_topic)
            this->mqtt->subscribe(cmd_topic.c_str());

            for (auto &topic : subscriptions)
            {
                this->mqtt->subscribe(topic);
            }

            this->reconnectCount++;

            return true;
        }
        else
        {
            mlogE("MQTT failed: %d", mqtt->state());
            return false;
        }

        return true;
    }

    // publishes a new message on a given topic with a given payload
    virtual void publishRaw(const char *topic, const char *payload, bool retained) override
    {
        if (this->network->getClient() == nullptr)
        {
            mlogE("Cannot publish, Client was not constructed!");
            return;
        }

        if (this->mqtt == nullptr)
        {
            mlogE("Cannot publish, MQTT Client was not constructed!");
            return;
        }

        if (!this->isMqttConfigured)
        {
            mlogE("Cannot publish, broker address is not configured");
            return;
        }

        if (!this->network->getClient()->connected())
        {
            mlogE("Cannot publish, not connected!");
            return;
        }

        mlogD("Publishing message on topic %s: %s", topic, payload);
        this->mqtt->publish(topic, payload, retained);
    }

    // publishes a new message on a Prefix_ABCDE/subtopic topic with
    // a given payload, allows to specify if payload should be retained
    virtual void publish(const char *subtopic, const char *payload, bool retained) override
    {
        char topic[60] = {0};
        sprintf(topic, "%s%s", this->mqttPrefix.c_str(), subtopic);

        this->publishRaw(topic, payload, retained);
    }

    // returns internal PubSubClient instance
    std::shared_ptr<PubSubClient> getPubSubClient()
    {
        return this->mqtt;
    }

    // subscribes to a given topic
    virtual void subscribe(const char *topic) override
    {
        subscriptions.push_back(topic);
        this->mqtt->subscribe(topic);
    }

    // unsubscribes from a given topic
    virtual void unsubscribe(const char *topic) override
    {
        auto it = std::find(subscriptions.begin(), subscriptions.end(), topic);
        if (it != subscriptions.end())
        {
            subscriptions.erase(it);
            this->mqtt->unsubscribe(topic);
        }
    }

    // internal function run when a new message is received
    // exposed only as a workaround
    void _mqttCommandReceived(char *topic, uint8_t *message, unsigned int length)
    {
        auto mokosh = Mokosh::getInstance();

        if (length > 64)
        {
            mlogE("MQTT message too long, ignoring.");
            return;
        }

        char msg[64] = {0};
        for (unsigned int i = 0; i < length; i++)
        {
            msg[i] = message[i];
        }
        msg[length + 1] = 0;

        if (String(topic) == cmd_topic)
        {
            mlogD("MQTT command: %s", msg);
            String command = String(msg);
            mokosh->_processCommand(command);
        }
        else
        {
            if (this->onMessage != nullptr)
            {
                this->onMessage(String(topic), message, length);
            }
            else
            {
                mlogW("MQTT message received, but no handler, ignoring.");
            }
        }
    }

private:
    std::shared_ptr<PubSubClient> mqtt;
    std::shared_ptr<MokoshNetworkService> network;

    bool isMqttConfigured = false;
    int reconnectCount = 0;
    String mqttPrefix;
    String clientId;

    std::vector<const char *> subscriptions;

    String cmd_topic;
};

#endif

#endif