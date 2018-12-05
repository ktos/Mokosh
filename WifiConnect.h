// WiFiConfig.h

#ifndef _WIFICONFIG_h
#define _WIFICONFIG_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

// tries connecting to the wireless network, configures internal host
// name by chip id
bool WiFi_connect(const char* ssid, const char* password, const char* prefix);

// returns internal host name
char* WiFi_getHostString();

#endif

