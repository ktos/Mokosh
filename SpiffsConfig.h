// SpiffsConfig.h

#ifndef _SPIFFSCONFIG_h
#define _SPIFFSCONFIG_h

#include "Arduino.h"

typedef struct Configuration {
	char ssid[32];
	char password[32];
	char broker[32];
	uint16_t brokerPort;
	char updateServer[32];
	uint16_t updatePort;
	float diffTemp;
	float diffHumid;
	float diffWeight;
	float weightScale;
	uint32_t color;
	char otaPath[32];
} Configuration;

bool SpiffsConfig_load(Configuration* config);
bool SpiffsConfig_update(Configuration config);
void SpiffsConfig_updateField(Configuration* config, const char* field, const char* value);
void SpiffsConfig_prettyPrint(Configuration config);
void SpiffsConfig_remove();
bool SpiffsConfig_exists();


#endif