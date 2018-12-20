#include <PubSubClient.h>
#include <EspSaveCrash.h>
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
	//sprintf(spay, "%f", payload);
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

void Mqtt_sendCrash(const char* topic)
{
    if (!mqtt_isInit) {
        Debug_print(DLVL_WARNING, "MOKOSH", "MQTT is not initialized");
        return;
    }

	// Note that 'EEPROM.begin' method is reserving a RAM buffer
	// The buffer size is SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_SPACE_SIZE
	EEPROM.begin(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_SPACE_SIZE);

	byte crashCounter = EEPROM.read(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_COUNTER);
	if (crashCounter == 0)
	{
		return;
	}

	char buf[100];

	int16_t readFrom = SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_DATA_SETS;
	for (byte k = 0; k < crashCounter; k++)
	{
		uint32_t crashTime;
		EEPROM.get(readFrom + SAVE_CRASH_CRASH_TIME, crashTime);
		sprintf(buf, "Crash # %d at %ld ms\n", k + 1, crashTime);
		mqtt.publish(topic, buf);

		sprintf(buf, "Reason of restart: %d\n", EEPROM.read(readFrom + SAVE_CRASH_RESTART_REASON));
		mqtt.publish(topic, buf);

		sprintf(buf, "Exception cause: %d\n", EEPROM.read(readFrom + SAVE_CRASH_EXCEPTION_CAUSE));
		mqtt.publish(topic, buf);

		uint32_t epc1, epc2, epc3, excvaddr, depc;
		EEPROM.get(readFrom + SAVE_CRASH_EPC1, epc1);
		EEPROM.get(readFrom + SAVE_CRASH_EPC2, epc2);
		EEPROM.get(readFrom + SAVE_CRASH_EPC3, epc3);
		EEPROM.get(readFrom + SAVE_CRASH_EXCVADDR, excvaddr);
		EEPROM.get(readFrom + SAVE_CRASH_DEPC, depc);
		sprintf(buf, "epc1=0x%08x epc2=0x%08x epc3=0x%08x excvaddr=0x%08x depc=0x%08x\n", epc1, epc2, epc3, excvaddr, depc);
		mqtt.publish(topic, buf);

		uint32_t stackStart, stackEnd;
		EEPROM.get(readFrom + SAVE_CRASH_STACK_START, stackStart);
		EEPROM.get(readFrom + SAVE_CRASH_STACK_END, stackEnd);

		int16_t currentAddress = readFrom + SAVE_CRASH_STACK_TRACE;
		int16_t stackLength = stackEnd - stackStart;
		uint32_t stackTrace;
		for (int16_t i = 0; i < stackLength; i += 0x10)
		{
			sprintf(buf, "%08x: ", stackStart + i);
			mqtt.publish(topic, buf);
			for (byte j = 0; j < 4; j++)
			{
				EEPROM.get(currentAddress, stackTrace);
				sprintf(buf, "%08x ", stackTrace);
				mqtt.publish(topic, buf);
				currentAddress += 4;
				if (currentAddress - SAVE_CRASH_EEPROM_OFFSET > SAVE_CRASH_SPACE_SIZE)
				{
					sprintf(buf, "\nIncomplete stack trace saved!");
					mqtt.publish(topic, buf);
					goto eepromSpaceEnd;
				}
			}
		}
	eepromSpaceEnd:
		readFrom = readFrom + SAVE_CRASH_STACK_TRACE + stackLength;
	}
	int16_t writeFrom;
	EEPROM.get(SAVE_CRASH_EEPROM_OFFSET + SAVE_CRASH_WRITE_FROM, writeFrom);
	EEPROM.end();

	// is there free EEPROM space avialable to save data for next crash?
	if (writeFrom + SAVE_CRASH_STACK_TRACE > SAVE_CRASH_SPACE_SIZE)
	{
		sprintf(buf, "No more EEPROM space available to save crash information!");
		mqtt.publish(topic, buf);
	}
	else
	{
		sprintf(buf, "EEPROM space available: 0x%04x bytes\n", SAVE_CRASH_SPACE_SIZE - writeFrom);
		mqtt.publish(topic, buf);
	}
}

