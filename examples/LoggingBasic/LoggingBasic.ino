#include <Mokosh.hpp>

Mokosh mokosh("Mokosh");

// callback for custom command handling
void customCommand(String command, String param)
{
    // you can use mlogI, mlogE and so on
    // to automatically work with RemoteDebug
    mlogI("Received command on command channel: %s", command.c_str());

    if (command == "hello")
    {
        mlogI("Hello.");
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
    mokosh.begin();
}

void loop()
{
    mokosh.loop();
}