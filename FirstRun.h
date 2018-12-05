// FirstRun.h

#ifndef _FIRSTRUN_h
#define _FIRSTRUN_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

void FirstRun_configure(const char* _version, const char* _informationalVersion, const char* _buildDate);

void FirstRun_start(char* prefix);

void FirstRun_handle();

#endif

