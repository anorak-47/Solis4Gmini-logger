//
// A simple server implementation with regex routes:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

const char* PARAM_MESSAGE = "message";

void onNotFoundRequest(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void setup() {

    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Hello, world");
    });

    // Send a GET request to <IP>/sensor/<number>
    server.on("^\\/sensor\\/([0-9]+)$", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String sensorNumber = request->pathArg(0);
        request->send(200, "text/plain", "Hello, sensor: " + sensorNumber);
    });
	
	// Send a GET request to <IP>/sensor/<number>/action/<action>
    server.on("^\\/sensor\\/([0-9]+)\\/action\//([a-zA-Z0-9]+)$", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String sensorNumber = request->pathArg(0);
        String action = request->pathArg(1);
        request->send(200, "text/plain", "Hello, sensor: " + sensorNumber + ", with action: " + action);
    });

    server.onNotFound(onNotFoundRequest);

    server.begin();
}

void loop() {
}
