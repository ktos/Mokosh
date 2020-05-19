#include "WifiConnect.h"

#include <ESP8266WiFi.h>

#include "Debug.h"

char hostString[16] = {0};

void WiFi_init(const char *prefix) {
    sprintf(hostString, "%s_%06X", prefix, ESP.getChipId());
}

bool WiFi_isInit() {
    return (hostString[0] != 0);
}

bool WiFi_connect(const char *ssid, const char *password) {
    WiFi.enableAP(0);

    Debug_print(DLVL_INFO, "WIFI", hostString);
    WiFi.hostname(hostString);

    Debug_print(DLVL_DEBUG, "WIFI", "Connecting...");
    WiFi.begin(ssid, password);

    int trials = 0;
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        Debug_print(DLVL_DEBUG, "WIFI", trials);
        trials++;
        delay(250);
    }

    // Check connection
    if (WiFi.status() == WL_CONNECTED) {
        Debug_print(DLVL_INFO, "WIFI", "Connected");
        Debug_print(DLVL_INFO, "WIFI", WiFi.localIP().toString());

        return true;
    } else {
        return false;
    }
}

char *WiFi_getHostString() {
    return hostString;
}