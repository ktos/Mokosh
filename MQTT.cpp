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

char debug_topic[32] = { 0 };
char version_topic[32] = { 0 };
char cmd_topic[32] = { 0 };

WiFiClient client;
PubSubClient mqtt(client);

bool Mqtt_isInit()
{
    return mqtt_isInit;
}

// function run when new message from the MQTT is sent
void Mqtt_onCommand(char* topic, uint8_t* message, unsigned int length)
{
	char msg[32] = { 0 };
	for (unsigned int i = 0; i < length; i++)
	{
		msg[i] = message[i];
	}
	msg[length + 1] = 0;

	Debug_print(DLVL_DEBUG, "COMMAND", msg);

	// if (strcmp(msg, "gver") == 0) {
	// 	char version_topic[32];
	// 	char* hostname = WiFi_getHostString();
	// 	sprintf(version_topic, "%s/version", hostname);
	// 	Mqtt_publish(version_topic, VERSION);

	// 	return;
	// }

	// if (strcmp(msg, "getfullver") == 0) {
	// 	Mqtt_publish(debug_topic, INFORMATIONAL_VERSION);

	// 	String buildInfo = (Debug_currentLevel() < 4) ? "DEBUG" : "RELEASE";

	// 	char buildInfoBuf[32];
	// 	buildInfo.toCharArray(buildInfoBuf, 32);
	// 	Mqtt_publish(debug_topic, buildInfoBuf);

	// 	return;
	// }

	if (strcmp(msg, "gmd5") == 0) {
		char md5[128];
		ESP.getSketchMD5().toCharArray(md5, 128);
		Mqtt_publish(debug_topic, md5);

		return;
	}

	// if (strcmp(msg, "getbuilddate") == 0) {
	// 	Mqtt_publish(debug_topic, BUILD_DATE);

	// 	return;
	// }

	if (strcmp(msg, "clearcrash") == 0) {
		SaveCrash.clear();

		return;
	}

	if (strcmp(msg, "getcrashcount") == 0) {
		int crashCount = SaveCrash.count();
		Mqtt_publish(debug_topic, crashCount);
		return;
	}

	if (strcmp(msg, "printcrashserial") == 0) {
		SaveCrash.print();
		return;
	}

	if (strcmp(msg, "getcrash") == 0) {
		Mqtt_sendCrash(debug_topic);

		return;
	}

	if (strcmp(msg, "reboot") == 0) {
		ESP.restart();

		return;
	}

	if (strcmp(msg, "factory") == 0) {
		Debug_print(DLVL_WARNING, "FACTORY", "Performing factory reset.");
		SpiffsConfig_remove();
		ESP.restart();

		return;
	}

	// if (strcmp(msg, "printconfigserial") == 0) {
	// 	Debug_print(DLVL_DEBUG, "CONFIG", "Printing configuration.");
	// 	SpiffsConfig_prettyPrint(config);

	// 	return;
	// }

	// if (strcmp(msg, "updateconfig") == 0) {
	// 	Debug_print(DLVL_INFO, "CONFIG", "Saving configuration to SPIFFS");
	// 	SpiffsConfig_update(config);

	// 	return;
	// }

	// if (strcmp(msg, "reloadconfig") == 0) {
	// 	Debug_print(DLVL_INFO, "CONFIG", "Reloading configuration from SPIFFS");
	// 	SpiffsConfig_load(&config);

	// 	return;
	// }

	if (msg[0] == 's') {
		String msg2 = String(msg);

		if (msg2.startsWith("showerror=")) {
			long errorCode = msg2.substring(10).toInt();
			Error_show(errorCode);
			return;
		}

		// if (msg2.startsWith("sota=")) {
		// 	String version = msg2.substring(5);

		// 	char buf[32];
		// 	version.toCharArray(buf, 32);

		// 	handleOTA(config, buf);
		// }

		// if (msg2.startsWith("setconfig=")) {
		// 	String data = msg2.substring(10);

		// 	int separator = data.indexOf(",");
		// 	String field = data.substring(0, separator);
		// 	String value = data.substring(separator + 1);

		// 	Debug_print(DLVL_DEBUG, "SETCONFIG", field);
		// 	Debug_print(DLVL_DEBUG, "SETCONFIG", value);

		// 	char field2[32];
		// 	field.toCharArray(field2, 32);

		// 	char value2[64];
		// 	value.toCharArray(value2, 64);

		// 	SpiffsConfig_updateField(&config, field2, value2);
		// }
	}
}

void Mqtt_setup(const char* broker, uint16_t port)
{
	Debug_print(DLVL_DEBUG, "MQTT", broker);
	brokerAddress.fromString(broker);
	brokerPort = port;
    mqtt.setCallback(Mqtt_onCommand);

    char* host = WiFi_getHostString();
    sprintf(debug_topic, "%s/debug", host);
    sprintf(cmd_topic, "%s/cmd", host);
    sprintf(version_topic, "%s/cmd", host);

    mqtt_isInit = true;
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
            Debug_print(DLVL_DEBUG, "MQTT", cmd_topic);
			mqtt.subscribe(cmd_topic);

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

