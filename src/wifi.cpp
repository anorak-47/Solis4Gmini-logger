#include "wifi.h"
#include "mqtt.h"
#include "led.h"
#include "config.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>

extern MQTTClient MQTTClient;
extern LED led;

constexpr unsigned long wifiTryReconnectInterval{30000};
static unsigned long prevReconnectTryTime{0};
static bool restart{false};

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

AsyncWebServer server(80);

void onWiFiConnect(const WiFiEventStationModeGotIP &event)
{
    (void)event;
    led.yellowOff();
    Serial.println("connected to WiFi");
    MQTTClient.begin();
}

void onWiFiDisconnect(const WiFiEventStationModeDisconnected &event)
{
    (void)event;
    led.yellowOn();
    Serial.println("disconnected from WiFi");
}

void onNotFoundRequest(AsyncWebServerRequest *request)
{
    request->send(404, F("text/plain"), F("not found"));
}

String onRestartRequest()
{
    restart = true;
    return F("restarting");
}

void wifi_setup()
{
    Serial.print(F("connecting to: "));
    Serial.println(WIFI_SSID);

#ifdef staticIP
    WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
#endif

    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(true);
    wifiConnectHandler = WiFi.onStationModeGotIP(onWiFiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWiFiDisconnect);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.hostname(HOSTNAME);

    uint16_t wifiCnt;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        led.yellowToggle();
        Serial.print(".");
        wifiCnt++;
        if (wifiCnt >= 240)
        {
        	led.yellowOn();
            delay(120000); // 120s
            led.yellowOff();
            ESP.restart(); // Restart ESP
        }
    }

    Serial.println(" ");
    Serial.println("Connected");

    /* Start AsyncWebServer */
    server.onNotFound(onNotFoundRequest);
    server.on("/api/restart", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", onRestartRequest()); });

    server.begin();

    // Start OTA
    ArduinoOTA.begin();
    ArduinoOTA.handle();
    ArduinoOTA.handle();
}

void wifi_loop()
{
    // Debug.handle();
	ArduinoOTA.handle();

    auto currentMillis{millis()};
    // if WiFi is down, try reconnecting
    if ((WiFi.status() != WL_CONNECTED) && (currentMillis - prevReconnectTryTime >= wifiTryReconnectInterval))
    {
        // debugE("Reconnecting to WiFi...");

        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);
        WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
        // WiFi.setHostname(WIFIHOSTNAME);
        // WiFi.begin(WIFISSID, WIFIPASSWORD);

        prevReconnectTryTime = currentMillis;
    }

    if (restart)
    {
        ESP.reset();
    }

    yield();
}
