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
    #define BUILD_DATE "1970-01-01"
#endif
