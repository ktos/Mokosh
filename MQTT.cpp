#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "MQTT.h"
#include "Debug.h"
#include "Error.h"
#include "WifiConnect.h"
#include "SpiffsConfig.h"

IPAddress brokerAddress;
uint16_t brokerPort;
bool mqtt_isInit = false;

WiFiClient client;
PubSubClient mqtt(client);

bool Mqtt_isInit()
{
    return mqtt_isInit;
}

void Mqtt_setup(const char* broker, uint16_t port)
{
	Debug_print(DLVL_DEBUG, "MQTT", broker);
	brokerAddress.fromString(broker);
	brokerPort = port;

    mqtt_isInit = true;
}

void Mqtt_subscribe(const char* topic)
{
    mqtt.subscribe(topic);
}


bool Mqtt_reconnect()
{
	mqtt.setServer(brokerAddress, brokerPort);
	uint8_t trials = 0;

	while (!client.connected()) {
		trials++;
		if (mqtt.connect(WiFi_getHostString())) {
			Debug_print(DLVL_DEBUG, "MQTT", "Connected");

			Debug_print(DLVL_DEBUG, "MQTT", "Subscribing");

			return true;
		}
		else {
			Debug_print(DLVL_ERROR, "MQTT", "Failed");
			Debug_print(DLVL_ERROR, "MQTT", mqtt.state());

			// if not connected in the third trial, give up
			if (trials > 3)
				return false;

			// wait 5 seconds before retrying
			delay(5000);
		}
	}
}

void Mqtt_publish(const char* topic, String payload)
{
    if (!mqtt_isInit) {
        Debug_print(DLVL_WARNING, "MOKOSH", "MQTT is not initialized");
        return;
    }

	int buflen = payload.length() + 1;
	char* buf = (char*)malloc(buflen);
	payload.toCharArray(buf, buflen);

	mqtt.publish(topic, buf);

	free(buf);
}

void Mqtt_publish(const char* topic, const char* payload)
{
    if (!mqtt_isInit) {
        Debug_print(DLVL_WARNING, "MOKOSH", "MQTT is not initialized");
        return;
    }

	mqtt.publish(topic, payload);
}

void Mqtt_publish(const char* topic, float payload)
{
	char spay[16];	
	dtostrf(payload, 4, 2, spay);

	Mqtt_publish(topic, spay);
}

bool Mqtt_isConnected()
{
    if (!mqtt_isInit) {
        Debug_print(DLVL_WARNING, "MOKOSH", "MQTT is not initialized");
        return false;
    }

	return mqtt.connected();
}

bool Mqtt_loop()
{
    if (!mqtt_isInit) {
        Debug_print(DLVL_WARNING, "MOKOSH", "MQTT is not initialized");
        return false;
    }

	return mqtt.loop();
}

void Mqtt_setCallback(void(*callback)(char*, uint8_t*, unsigned int))
{
    if (!mqtt_isInit) {
        Debug_print(DLVL_WARNING, "MOKOSH", "MQTT is not initialized");
        return;
    }

    mqtt.setCallback(callback);
}
