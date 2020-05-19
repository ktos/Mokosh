#include "FirstRun.h"

#include <WiFiClient.h> 
#include <FS.h>
#include "Debug.h"
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

char ssid[16] = { 0 };
char version[32] = { 0 };
char informationalVersion[100] = { 0 };
char buildDate[11] = { 0 };

void FirstRun_configure(const char* _version, const char* _informationalVersion, const char* _buildDate) {
	strcpy(version, _version);
	strcpy(informationalVersion, _informationalVersion);
	strcpy(buildDate, _buildDate);
}


void handleRoot() {
	server.send(200, "text/plain", "Hi, I'm Mokosh.");
}

File fsUploadFile;

void handleFileUpload() {
	HTTPUpload& upload = server.upload();
	if (upload.status == UPLOAD_FILE_START) {

		fsUploadFile = SPIFFS.open("/config.json", "w");

		Debug_print(DLVL_DEBUG, "FIRSTRUN-CONFIG", "Upload started");
	}
	else if (upload.status == UPLOAD_FILE_WRITE) {
		if (fsUploadFile)
			fsUploadFile.write(upload.buf, upload.currentSize);
	}
	else if (upload.status == UPLOAD_FILE_END) {
		if (fsUploadFile)
			fsUploadFile.close();

		Debug_print(DLVL_DEBUG, "FIRSTRUN-CONFIG", "Upload finished");
		Debug_print(DLVL_DEBUG, "FIRSTRUN-CONFIG", (int)upload.totalSize);
	}
}

void handleWhois() {

	String whois = "{ ";
	whois += "\"id\": \"";
	whois += ssid;
	whois += "\",\n";

	whois += "\"version\": \"";
	whois += version;
	whois += "\",\n";

#if DEBUG_LEVEL < 4
	whois += "\"informational_version\": \"";
	whois += informationalVersion;
	whois += "\",\n";
#endif

#if DEBUG_LEVEL < 4
	whois += "\"build_date\": \"";
	whois += buildDate;
	whois += "\",\n";
#endif

	whois += "\"md5\": \"";
	whois += ESP.getSketchMD5();
	whois += "\"\n";

	whois += " }";

	server.send(200, "application/json", whois);
}

void handleNotFound() {
	String message = "File Not Found\n\n";

#if DEBUG_LEVEL < 4
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += (server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for (uint8_t i = 0; i < server.args(); i++) {
		message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
	}
#endif

	server.send(404, "text/plain", message);
}

void handleReboot() {
	server.send(200, "text/plain", "Good bye.");

	delay(1000);
	server.close();
	server.stop();
	WiFi.disconnect(true);
	delay(1000);

	ESP.restart();
}

void FirstRun_start(char* prefix)
{
	sprintf(ssid, "%s_%06X", prefix, ESP.getChipId());

	WiFi.softAP(ssid);

	IPAddress myIP = WiFi.softAPIP();

	Debug_print(DLVL_INFO, "FIRSTRUN", "Started First Run");
	Debug_print(DLVL_INFO, "FIRSTRUN", myIP.toString());

	server.onNotFound(handleNotFound);
	server.on("/", handleRoot);
	server.on("/whois", handleWhois);
	server.on("/reboot", HTTPMethod::HTTP_POST, handleReboot);
	server.on("/config", HTTPMethod::HTTP_POST, []() { server.send(200, "text/plain", ""); }, handleFileUpload);

	server.begin();
}

void FirstRun_handle()
{
	server.handleClient();
}