#include "Arduino.h"
#include <ArduinoOTA.h>

typedef std::function<void(void)> THandlerFunction;
typedef std::function<void(String, String)> THandlerFunction_Command;
typedef std::function<void(String, uint8_t *msg, unsigned int length)> THandlerFunction_Message;

typedef std::function<void(int)> THandlerFunction_MokoshError;

class MokoshWiFiHandlers
{
public:
    THandlerFunction onConnect;
    THandlerFunction onConnectFail;
    THandlerFunction onDisconnect;
};

class MokoshEvents
{
public:
    THandlerFunction onReconnectRequested;
};