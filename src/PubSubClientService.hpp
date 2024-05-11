#ifndef PUBSUBCLIENTSERVICE_H
#define PUBSUBCLIENTSERVICE_H

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
            mdebugE("MQTT configuration is not provided!");
            return false;
        }
        uint16_t brokerPort = mokosh->config->get<int>(mokosh->config->key_broker_port, 1883);

        // it it is an IP address
        if (broker.fromString(brokerAddress))
        {
            this->mqtt->setServer(broker, brokerPort);
            mdebugV("MQTT broker set to %s port %d", broker.toString().c_str(), brokerPort);
        }
        else
        {
            // so it must be a domain name
            this->mqtt->setServer(brokerAddress.c_str(), brokerPort);
            mdebugV("MQTT broker set to %s port %d", brokerAddress.c_str(), brokerPort);
        }

        this->isMqttConfigured = true;
        this->mqttPrefix = mokosh->getMqttPrefix();

        this->clientId = mokosh->getHostNameWithPrefix();
        if (mokosh->config->hasKey(mokosh->config->key_client_id))
            this->clientId = mokosh->config->get<String>(mokosh->config->key_client_id, mokosh->getHostNameWithPrefix());

        this->setupReady = true;
        this->cmd_topic = mokosh->getMqttPrefix() + String(cmd);

        return reconnect();
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
        if (this->mqtt->connect(clientId.c_str()))
        {
            mdebugI("MQTT reconnected");

            this->mqtt->setCallback([&](char *topic, uint8_t *message, unsigned int length)
                                    { this->_mqttCommandReceived(topic, message, length); });

            // resubscribing to all subscribed topics (and cmd_topic)
            this->mqtt->subscribe(cmd_topic.c_str());

            for (auto &topic : subscriptions)
            {
                this->mqtt->subscribe(topic);
            }

            return true;
        }
        else
        {
            // TODO: use resiliency

            // if (this->isIgnoringConnectionErrors)
            // {
            //     mdebugD("Client not connected, but ignoring.");
            //     return false;
            // }

            mdebugE("MQTT failed: %d", mqtt->state());

            // if not connected in the third trial, give up
            // if (trials > 3)
            //     return false;

            // wait 5 seconds before retrying
            delay(5000);
        }

        return true;
    }

    // publishes a new message on a given topic with a given payload
    virtual void publishRaw(const char *topic, const char *payload, bool retained) override
    {
        if (this->network->getClient() == nullptr)
        {
            mdebugE("Cannot publish, Client was not constructed!");
            return;
        }

        if (this->mqtt == nullptr)
        {
            mdebugE("Cannot publish, MQTT Client was not constructed!");
            return;
        }

        if (!this->isMqttConfigured)
        {
            mdebugE("Cannot publish, broker address is not configured");
            return;
        }

        if (!this->network->getClient()->connected())
        {
            mdebugE("Cannot publish, not connected!");
            return;
        }

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
            mdebugE("MQTT message too long, ignoring.");
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
            mdebugD("MQTT command: %s", msg);
            String command = String(msg);
            mokosh->_processCommand(command);
        }
        else
        {
            if (mokosh->onMessage != nullptr)
            {
                mokosh->onMessage(String(topic), message, length);
            }
            else
            {
                mdebugW("MQTT message received, but no handler, ignoring.");
            }
        }
    }

private:
    std::shared_ptr<PubSubClient> mqtt;
    std::shared_ptr<MokoshNetworkService> network;

    bool isMqttConfigured = false;
    String mqttPrefix;
    String clientId;

    const char *cmd = "cmd";
    std::vector<const char *> subscriptions;

    String cmd_topic;
};

#endif