#include <Mokosh.h>

void setup() {
    Debug_init(DLVL_INFO);    
}

void loop() {
    // won't be shown, as minimal debug level is INFO
    Debug_print(DLVL_DEBUG, "Topic", "Some debug information");

    // showing some values
    Debug_print(DLVL_INFO, "Values", 2.0f);

    // showing some errors
    Debug_print(DLVL_ERROR, "Errors", 21);

    delay(1000);
}