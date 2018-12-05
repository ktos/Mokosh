#include "OTA.h"
#include <ESP8266httpUpdate.h>
#include "SpiffsConfig.h"
#include "Debug.h"

void handleOTA(Configuration config, const char* version)
{
	char uri[128];
	sprintf(uri, config.otaPath, version);

	Debug_print(DLVL_INFO, "OTA", "Starting OTA update");
	Debug_print(DLVL_DEBUG, "OTA", config.updateServer);
	Debug_print(DLVL_DEBUG, "OTA", config.updatePort);
	Debug_print(DLVL_DEBUG, "OTA", uri);

	t_httpUpdate_return ret = ESPhttpUpdate.update(config.updateServer, config.updatePort, uri);

	if (ret == HTTP_UPDATE_FAILED) {
		Debug_print(DLVL_WARNING, "OTA", "Update failed");
	}
}