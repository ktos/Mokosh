#include "Reading.h"

#include <Mokosh.h>

// updates the last reading if new value if different than the previous value by defined
// difference and publishes the MQTT topic if true
void updateReading(float *oldVal, float newVal, float diff, const char *topic) {
    if (checkBound(newVal, *oldVal, diff)) {
        *oldVal = newVal;
        Debug_print(DLVL_DEBUG, "MOKOSH", "New reading");
        Debug_print(DLVL_DEBUG, "MOKOSH", topic);
        Debug_print(DLVL_DEBUG, "MOKOSH", newVal);

        Mqtt_publish(topic, newVal);
    }
}

// checks if the difference between new and previous value is greater than defined
// difference
bool checkBound(float newValue, float prevValue, float maxDiff) {
    return !isnan(newValue) &&
           (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}