// Debug.h

#ifndef _DEBUG_h
#define _DEBUG_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

const uint8_t DLVL_DEBUG = 1;
const uint8_t DLVL_INFO = 2;
const uint8_t DLVL_WARNING = 4;
const uint8_t DLVL_ERROR = 8;

void Debug_init(uint8_t minlevel);

uint8_t Debug_currentLevel();

void Debug_print(uint8_t level, const char* topic, const char* message);

void Debug_print(uint8_t level, const char* topic, String message);

void Debug_print(uint8_t level, const char* topic, float value);

void Debug_print(uint8_t level, const char* topic, int value);

void Debug_print(uint8_t level, const char* topic, uint32_t value);

#endif

