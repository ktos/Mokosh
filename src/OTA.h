// OTA.h

#ifndef _OTA_h
#define _OTA_h

#include "Arduino.h"
#include "SpiffsConfig.h"

// handles the OTA update by using the configuration and updating to
// the desired version
void handleOTA(Configuration config, const char *version);

#endif
