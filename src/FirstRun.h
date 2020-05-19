// FirstRun.h

#ifndef _FIRSTRUN_h
#define _FIRSTRUN_h

#include "Arduino.h"

void FirstRun_configure(const char *_version, const char *_informationalVersion, const char *_buildDate);

void FirstRun_start(char *prefix);

void FirstRun_handle();

#endif
