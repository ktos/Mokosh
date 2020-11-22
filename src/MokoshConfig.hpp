#include "Arduino.h"

typedef struct MokoshConfiguration {
    char ssid[32];
    char password[32];
    char broker[32];
    uint16_t brokerPort;
    char updateServer[32];
    uint16_t updatePort;    
    char updatePath[32];
} MokoshConfiguration;

#ifndef VERSION
    #define VERSION "1.0.0"
    #define INFORMATIONAL_VERSION "1.0.0"
    #define BUILD_DATE "2020-11-20"
#endif

// initializes a SPIFFS access
// bool SpiffsConfig_begin();
// bool SpiffsConfig_load(MokoshConfiguration *config);
// bool SpiffsConfig_update(MokoshConfiguration config);
// void SpiffsConfig_updateField(MokoshConfiguration *config, const char *field, const char *value);
// void SpiffsConfig_prettyPrint(MokoshConfiguration config);
// void SpiffsConfig_remove();
// bool SpiffsConfig_exists();