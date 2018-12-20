// BrokerDiscovery.h

#ifndef _MQTT_h
#define _MQTT_h

#include "Arduino.h"

// returns if we are connected to the MQTT broker
bool Mqtt_isConnected();

// handles MQTT subscription
bool Mqtt_loop();

// sets up values for MQTT connection: broker address and port
void Mqtt_setup(const char* broker, uint16_t port);

// (re)connects to the MQTT broker and subscribes
// to the command topic
bool Mqtt_reconnect();

// publishes any string message to the MQTT topic
void Mqtt_publish(const char* topic, const char* payload);

// publishes float (sensor data) to the MQTT topic
void Mqtt_publish(const char* topic, float payload);

// sets the function run when new message is being sent to subscribed
// topic
void Mqtt_setCallback(void(*callback)(char*, uint8_t*, unsigned int));

// sends crash data to the MQTT server, line by line
void Mqtt_sendCrash(const char* topic);

// returns if MQTT system has been initialized
bool Mqtt_isInit();

#endif