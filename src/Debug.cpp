#include "Debug.h"

uint8_t minlevel;
bool debug_isInit = false;

uint8_t Debug_currentLevel() {
    return minlevel;
}

void Debug_init(uint8_t minl) {
    minlevel = minl;
    debug_isInit = true;
    Serial.begin(115200);
}

bool Debug_debug_isInit() {
    return debug_isInit;
}

void Debug_print(uint8_t level, const char *topic, const char *message) {
    if (!debug_isInit)
        return;

    if (level < minlevel)
        return;

    Serial.print(level);
    Serial.print(" ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(message);
}

void Debug_print(uint8_t level, const char *topic, String message) {
    if (!debug_isInit)
        return;

    if (level < minlevel)
        return;

    Serial.print(level);
    Serial.print(" ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(message);
}

void Debug_print(uint8_t level, const char *topic, float value) {
    if (!debug_isInit)
        return;

    char result[8];
    dtostrf(value, 6, 2, result);

    Debug_print(level, topic, result);
}

void Debug_print(uint8_t level, const char *topic, int value) {
    if (!debug_isInit)
        return;

    char result[8];
    sprintf(result, "%d", value);

    Debug_print(level, topic, result);
}

void Debug_print(uint8_t level, const char *topic, uint32_t value) {
    if (!debug_isInit)
        return;

    char result[16];
    sprintf(result, "%u", value);

    Debug_print(level, topic, result);
}