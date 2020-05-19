#include <Mokosh.h>

char debug_topic[32] = { 0 };

void setup() {
    srand(millis());

    // connects to the WiFi network called yourssid with password yourpassword
    // setting hostname to Mokosh_XXXXX
    WiFi_init("Mokosh");
    WiFi_connect("yourssid", "yourpassword");

    // will connect to MQTT broker at 192.168.1.10 at port 1883
    Mqtt_setup("192.168.1.10", 1883);
    char* host = WiFi_getHostString();
    Mqtt_reconnect();

    // will publish to topic Mokosh_XXXXX/debug
    sprintf(debug_topic, "%s/debug", host);

    // sets up callback run when new command is received
    Mqtt_setCallback(onCommand);

    // sets up debug communiation over serial
    Debug_init(DLVL_DEBUG);
}

long lastMessage = 0;

void loop() {
    // publish new random int message every 2000 ms
    if (millis() - lastMessage > 2000) {
        Mqtt_publish(debug_topic, rand());
        lastMessage = millis();
    }

    // handle MQTT communication
    Mqtt_loop();
}

void onCommand(char* topic, uint8_t* message, unsigned int length)
{
	char msg[32] = { 0 };
	for (unsigned int i = 0; i < length; i++)
	{
		msg[i] = message[i];
	}
	msg[length + 1] = 0;

	Debug_print(DLVL_DEBUG, "I received command!", msg);
}