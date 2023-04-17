#ifndef MAIN_H
#define MAIN_H

// Include libraries
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ModbusMaster.h>

#include <Ticker.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <AddrList.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>

#include <PubSubClient.h>
#include <InfluxDb.h>

#include <TimeLib.h>
#include <NTPClient.h>

#include <WiFiClientSecure.h>

#include "config.h"
#include "modbus.h"
#include "inverter.h"
#include "energymonitor.h"
#include "mqtt.h"
#include "myTicker.h"
#include "time.h"
#include "led.h"
#include "ds18b20.h"

#ifdef IPv6
#include <AddrList.h>
#include <lwip/dns.h>
#endif

extern "C" {
#include "user_interface.h"
}



#endif
