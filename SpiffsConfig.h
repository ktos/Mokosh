// SpiffsConfig.h

#ifndef _SPIFFSCONFIG_h
#define _SPIFFSCONFIG_h

#include "Arduino.h"

typedef struct MokoshConfiguration {
	char ssid[32];
	char password[32];
	char broker[32];
	uint16_t brokerPort;
	char updateServer[32];
	uint16_t updatePort;	
	uint32_t color;
	char updatePath[32];
} Configuration;

// initializes a SPIFFS access
bool SpiffsConfig_begin();
bool SpiffsConfig_load(MokoshConfiguration* config);
bool SpiffsConfig_update(MokoshConfiguration config);
void SpiffsConfig_updateField(MokoshConfiguration* config, const char* field, const char* value);
void SpiffsConfig_prettyPrint(MokoshConfiguration config);
void SpiffsConfig_remove();
bool SpiffsConfig_exists();


#endif