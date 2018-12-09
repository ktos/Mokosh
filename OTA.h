// OTA.h

#ifndef _OTA_h
#define _OTA_h

#include "SpiffsConfig.h"
#include "Arduino.h"

// handles the OTA update by using the configuration and updating to
// the desired version
void handleOTA(Configuration config, const char* version);

#endif

