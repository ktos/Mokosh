#include <Mokosh.hpp>

Mokosh mokosh;

// callback for custom command handling
void customCommand(uint8_t* message, unsigned int length) {
    // you can use mdebugI, mdebugE and so on
    // to automatically work with RemoteDebug
    mdebugI("Received message on command channel of length %d", length);

    if (length < 32) {
        // very crude string creating from bytes
        char msg[32] = {0};
        for (unsigned int i = 0; i < length; i++) {
            msg[i] = message[i];
        }
        msg[length + 1] = 0;

        mdebugD("Message: %s", msg);
    } else {
        mdebugE("Message too long for buffer.");
    }
}

void setup() {
    // debug set to Verbose will display everything
    // available are: verbose, debug, info, warning, error
    mokosh.setDebugLevel(DebugLevel::VERBOSE);
    mokosh.onCommand = customCommand;

    // no custom configuration is set, so it will try to load
    // from /config.json in LittleFS
    mokosh.begin("Mokosh");
}

void loop() {
    mokosh.loop();
}