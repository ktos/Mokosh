#include "Arduino.h"
#include <ArduinoOTA.h>

class MokoshOTAConfiguration {
    typedef std::function<void(void)> THandlerFunction;
	typedef std::function<void(ota_error_t)> THandlerFunction_Error;
	typedef std::function<void(unsigned int, unsigned int)> THandlerFunction_Progress;
    public:
        char passwordHash[32];
        uint16_t port = 3232;
        THandlerFunction onStart;
        THandlerFunction onEnd;
        THandlerFunction_Error onError;
        THandlerFunction_Progress onProgress;
};