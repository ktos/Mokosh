// updates the last reading if new value if different than the previous value by defined
// difference and publishes the MQTT topic if true
void updateReading(float* oldVal, float newVal, float diff, const char* topic);

// checks if the difference between new and previous value is greater than defined
// difference
bool checkBound(float newValue, float prevValue, float maxDiff);