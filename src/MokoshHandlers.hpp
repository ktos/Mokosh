#include "Arduino.h"
#include <ArduinoOTA.h>

typedef std::function<void(void)> THandlerFunction;
typedef std::function<void(String)> THandlerFunction_Command;
typedef std::function<void(String, uint8_t *msg, unsigned int length)> THandlerFunction_Message;

typedef std::function<void(int)> THandlerFunction_MokoshError;

class MokoshOTAHandlers
{
    typedef std::function<void(ota_error_t)> THandlerFunction_OtaError;
    typedef std::function<void(unsigned int, unsigned int)> THandlerFunction_Progress;

public:
    THandlerFunction onStart;
    THandlerFunction onEnd;
    THandlerFunction_OtaError onError;
    THandlerFunction_Progress onProgress;
};

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