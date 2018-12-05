// ESPError.h

#ifndef _ESPERROR_h
#define _ESPERROR_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#define ERROR_CONFIG 10
#define ERROR_SPIFFS 50
#define ERROR_WIFI 100
#define ERROR_DISCOVERY 500
#define ERROR_SENSOR 1000
#define ERROR_BROKER 2000
#define ERROR_CUSTOM 10000

#define TRIALS 3

// configures if built-in ESP led should be used
// for signalizing errors. should be run in the setup()
// phase
void Error_configure(bool useBuiltInLed, bool useNeoPixel);

// shows error for the specific error code, by blinking
// the built in led or another led with delays between
// blinks coresponding to the error code
void Error_show(uint16_t errorCode);

#endif

