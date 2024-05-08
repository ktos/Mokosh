#include <Mokosh.hpp>

Mokosh mokosh;

// callback for custom command handling
void customCommand(String command)
{
    // you can use mdebugI, mdebugE and so on
    // to automatically work with RemoteDebug
    mdebugI("Received command on command channel: %s", command.c_str());

    if (command == "hello")
    {
        mdebugI("Hello.");
    }
}

void setup()
{
    // debug set to Verbose will display everything
    // available are: verbose, debug, info, warning, error
    mokosh.setLogLevel(LogLevel::VERBOSE);
    mokosh.onCommand = customCommand;

    // no custom configuration is set, so it will try to load
    // from /config.json in LittleFS
    mokosh.begin("Mokosh");
}

void loop()
{
    mokosh.loop();
}