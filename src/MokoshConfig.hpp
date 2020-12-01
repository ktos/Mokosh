#include "Arduino.h"

typedef struct MokoshConfiguration {
    char ssid[32];
    char password[32];
    char broker[32];
    uint16_t brokerPort;
} MokoshConfiguration;