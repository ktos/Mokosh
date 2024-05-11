#include "Arduino.h"

typedef std::function<void(void)> THandlerFunction;
typedef std::function<void(String, String)> THandlerFunction_Command;

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
    // happens only if Wi-Fi is not used and network connection is requested
    THandlerFunction onReconnectRequested;
};