#include "BuildConfig.h"
#include "BuildVersion.h"

#include <Mokosh.h>
#include "Reading.h"

// Framework Configuration object
MokoshConfiguration config;

// values acquired from sensors
float randv;

// MQTT topics
char rand_topic[32] = { 0 };

char heartbeat_topic[32] = { 0 };
char debug_topic[32] = { 0 };
char cmd_topic[32] = { 0 };

// when the last message was sent
long lastMsg = 0;

// when the last heartbeat message was sent
long lastHeartbeat = 0;

bool firstRun = false;

void setup(void) {
	Debug_init(DEBUG_LEVEL);	

#ifdef ENABLE_NEOPIXEL	
	Error_configure(false, true);

	NeoPixel_setup();
	NeoPixel_anim(KnightRider1, Colors::BrightRed);
	delay(100);
	NeoPixel_anim(KnightRider2, Colors::BrightRed);
#else
	Error_configure(true, false);
#endif

#ifdef ENABLE_NEOPIXEL
	NeoPixel_progress(1, Colors::Green);
#endif
	if (!SpiffsConfig_begin()) {
		Error_show(ERROR_SPIFFS);
	}

	if (!SpiffsConfig_exists()) {
		Debug_print(DLVL_DEBUG, "FIRSTRUN", "No config file, switching to first run configuration");
		firstRun = true;
	}

	if (firstRun) {
		FirstRun_start(DEV_PREFIX);
		return;
	}

	if (!SpiffsConfig_load(&config)) {
		Error_show(ERROR_CONFIG);
	}	

#ifdef ENABLE_NEOPIXEL
	NeoPixel_progress(2, Colors::Green);
#endif
	WiFi_init(DEV_PREFIX);

	if (!WiFi_connect(config.ssid, config.password)) {
		Error_show(ERROR_WIFI);
	}
#ifdef ENABLE_NEOPIXEL
	NeoPixel_progress(3, Colors::Green);
#endif

	Mqtt_setup(config.broker, config.brokerPort);
#ifdef ENABLE_NEOPIXEL
	NeoPixel_progress(4, Colors::Green);
#endif

	Debug_print(DLVL_DEBUG, "MQTT", "connecting");	

	if (!Mqtt_reconnect()) {
		Error_show(ERROR_BROKER);
	}
	Mqtt_setCallback(onCommand);
#ifdef ENABLE_NEOPIXEL
	NeoPixel_progress(5, Colors::Green);
#endif

	char* hostname = WiFi_getHostString();
	sprintf(rand_topic, "%s/rand", hostname);
	
	sprintf(heartbeat_topic, "%s/debug/heartbeat", hostname);
	sprintf(debug_topic, "%s/debug", hostname);
	sprintf(cmd_topic, "%s/cmd", hostname);
	Mqtt_subscribe(cmd_topic);

	char version_topic[32];
	sprintf(version_topic, "%s/version", hostname);
	Debug_print(DLVL_INFO, "MOKOSH", "Sending hello");
	Mqtt_publish(version_topic, VERSION);
#ifdef ENABLE_NEOPIXEL
	NeoPixel_progress(6, Colors::Green);
#endif

	Debug_print(DLVL_INFO, "MOKOSH", "Beginning");

#ifdef ENABLE_RAND
	// some kind of sensor initialization, taring, getting first value and so on
#endif

#ifdef ENABLE_NEOPIXEL
	NeoPixel_progress(7, Colors::Green);
#endif

	randv = -127.0f;	

	// getting initial values
#ifdef ENABLE_RAND
	updateReading(&randv, rand() % 1000, 0, rand_topic);	
#endif

#ifdef ENABLE_NEOPIXEL
	NeoPixel_progress(1, Colors::Green);
#endif

	Debug_print(DLVL_DEBUG, "MOKOSH", "Initial values:");
	Debug_print(DLVL_DEBUG, "MOKOSH", randv);	

	Debug_print(DLVL_INFO, "MOKOSH", "Ready.");

#ifdef ENABLE_NEOPIXEL
	NeoPixel_color(Colors::Black);
#endif
}

// function run when new message from the MQTT is sent
void onCommand(char* topic, uint8_t* message, unsigned int length)
{
	char msg[32] = { 0 };
	for (unsigned int i = 0; i < length; i++)
	{
		msg[i] = message[i];
	}
	msg[length + 1] = 0;

	Debug_print(DLVL_DEBUG, "COMMAND", msg);

#ifdef ENABLE_DHT
	if (strcmp(msg, "getr") == 0) {
		updateReading(&randv, rand(), 0, rand_topic);

		return;
	}	
#endif

	if (strcmp(msg, "led1") == 0) {
#ifdef ENABLE_NEOPIXEL
		NeoPixel_color(config.color);
#else
		digitalWrite(LED_BUILTIN, LOW);
#endif

		return;
	}

	if (strcmp(msg, "led0") == 0) {
#ifdef ENABLE_NEOPIXEL
		NeoPixel_color(Colors::Black);
#else
		digitalWrite(LED_BUILTIN, HIGH);
#endif

		return;
	}

	if (strcmp(msg, "gver") == 0) {
		char version_topic[32];
		char* hostname = WiFi_getHostString();
		sprintf(version_topic, "%s/version", hostname);
		Mqtt_publish(version_topic, VERSION);

		return;
	}

	if (strcmp(msg, "getfullver") == 0) {		
		Mqtt_publish(debug_topic, INFORMATIONAL_VERSION);

		String buildInfo = (Debug_currentLevel() < 4) ? "DEBUG" : "RELEASE";

#ifdef ENABLE_RAND
		buildInfo += "+RAND";
#endif

#ifdef ENABLE_NEOPIXEL
		buildInfo += "+NEOPIXEL";
#endif		

		char buildInfoBuf[32];
		buildInfo.toCharArray(buildInfoBuf, 32);
		Mqtt_publish(debug_topic, buildInfoBuf);

		return;
	}

	if (strcmp(msg, "gmd5") == 0) {
		char md5[128];
		ESP.getSketchMD5().toCharArray(md5, 128);
		Mqtt_publish(debug_topic, md5);

		return;
	}

	if (strcmp(msg, "getbuilddate") == 0) {		
		Mqtt_publish(debug_topic, BUILD_DATE);

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

	if (strcmp(msg, "printconfigserial") == 0) {		
		Debug_print(DLVL_DEBUG, "CONFIG", "Printing configuration.");
		SpiffsConfig_prettyPrint(config);

		return;
	}

	if (strcmp(msg, "updateconfig") == 0) {
		Debug_print(DLVL_INFO, "CONFIG", "Saving configuration to SPIFFS");
		SpiffsConfig_update(config);

		return;
	}

	if (strcmp(msg, "reloadconfig") == 0) {
		Debug_print(DLVL_INFO, "CONFIG", "Reloading configuration from SPIFFS");
		SpiffsConfig_load(&config);

		return;
	}

	if (msg[0] == 's') {
		String msg2 = String(msg);		

		if (msg2.startsWith("showerror=")) {
			long errorCode = msg2.substring(10).toInt();
			Error_show(errorCode);
			return;
		}

#ifdef ENABLE_NEOPIXEL
		if (msg2.startsWith("setcolor=")) {
			String color2 = msg2.substring(9);

			if (color2.length() == 11) {
				int r = color2.substring(0, 3).toInt();
				int g = color2.substring(4, 7).toInt();
				int b = color2.substring(8, 11).toInt();

				config.color = NeoPixel_convertColorToCode(r, g, b);
			}
		}

		if (msg2.startsWith("showcolor=")) {
			String color2 = msg2.substring(10);

			if (color2.length() == 11) {
				int r = color2.substring(0, 3).toInt();
				int g = color2.substring(4, 7).toInt();
				int b = color2.substring(8, 11).toInt();

				uint32_t color = NeoPixel_convertColorToCode(r, g, b);
				NeoPixel_color(color);
			}

			return;
		}
#endif

		if (msg2.startsWith("sota=")) {
			String version = msg2.substring(5);

			char buf[32];
			version.toCharArray(buf, 32);

			handleOTA(config, buf);
		}

		if (msg2.startsWith("setconfig=")) {
			String data = msg2.substring(10);

			int separator = data.indexOf(",");
			String field = data.substring(0, separator);
			String value = data.substring(separator + 1);
		
			Debug_print(DLVL_DEBUG, "SETCONFIG", field);
			Debug_print(DLVL_DEBUG, "SETCONFIG", value);

			char field2[32];
			field.toCharArray(field2, 32);

			char value2[64];
			value.toCharArray(value2, 64);

			SpiffsConfig_updateField(&config, field2, value2);
		}
	}
}

void loop(void)
{
	if (firstRun) {
#ifdef ENABLE_NEOPIXEL
		NeoPixel_anim(RainbowCycle, 0);
#endif
		FirstRun_handle();

		return;
	}

	if (!Mqtt_isConnected()) {
		Debug_print(DLVL_WARNING, "MQTT", "Reconnecting");		
		Mqtt_reconnect();
	}

	Mqtt_loop();

	long now = millis();

	if (now - lastHeartbeat > HEARTBEAT_TIME) {
		lastHeartbeat = now;
		Mqtt_publish(heartbeat_topic, now);
	}

	if (now - lastMsg > UPDATE_TIME) {
		lastMsg = now;
#ifdef ENABLE_RAND
		updateReading(&randv, rand() % 1000, 100, rand_topic);		
#endif
	}

	delay(100);
}