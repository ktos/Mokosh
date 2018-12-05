// OTA.h

#ifndef _OTA_h
#define _OTA_h

#include "SpiffsConfig.h"

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

// handles the OTA update by using the configuration and updating to
// the desired version
void handleOTA(Configuration config, const char* version);

#endif

