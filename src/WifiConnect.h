// WiFiConfig.h

#ifndef _WIFICONFIG_h
#define _WIFICONFIG_h

#include "Arduino.h"

// tries connecting to the wireless network
bool WiFi_connect(const char *ssid, const char *password);

// returns internal host name
char *WiFi_getHostString();

// configures internal host name by chip id
void WiFi_init(const char *prefix);

// returns if WiFi has been initialized
bool WiFi_isInit();

#endif
