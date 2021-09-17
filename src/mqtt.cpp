#include "mqtt.h"

#ifdef MQTT
WiFiClient net;
PubSubClient client(net);
#endif

// TODO: fix this stuff

// void reconnect();
// void connect();
// void mqtt_callback(char *topic, byte *payload, unsigned int length);

myMqtt::myMqtt()
{
}

/*
Starts MQTT
*/
void myMqtt::begin()
{
    Serial.println(F("Connecting to Mqtt..."));
    connect();
}

/*
Loops MQTT
*/
void myMqtt::loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();
}

/*
Send values over MQTT
*/
void myMqtt::sendPayload(String const &path, String const &payload)
{
    client.publish(path.c_str(), payload.c_str());
}

void myMqtt::setModbusSlave(ModbusSlaveDevice &device)
{
    modbusSlave = &device;
}

/*
Sends status
*/
void myMqtt::sendStatus()
{
    char bufferSend[10];
    dtostrf(WiFi.RSSI(), 0, 0, bufferSend);
    client.publish(mqtt_status_rssi_topic, bufferSend);
}

void myMqtt::mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    String sTopic{topic};

    payload[length] = '\0';
    String payloadStr(reinterpret_cast<char *>(payload));

    if (!modbusSlave)
        return;

    if (!sTopic.startsWith(F(mqtt_modbus_set_holdingregister)))
        return;

    auto registers{modbusSlave->getRegisterValues()};

    ModbusRegisterDescription regDesc;
    String topicbase{F(mqtt_modbus_set_holdingregister)};

    std::for_each(std::begin(registers), std::end(registers),
                  [&topicbase, &sTopic, &regDesc, &payloadStr, this](auto const &reg)
                  {
                      memcpy_P(&regDesc, reg.second.reg, sizeof(ModbusRegisterDescription));
                      auto mqttTopic{String{regDesc.mqttTopic}};
                      if (topicbase + mqttTopic == sTopic)
                      {
                          auto value{strtol(payloadStr.c_str(), nullptr, 10)};
                          auto result{modbusSlave->writeHoldingRegister(regDesc, value)};

                          auto msg{mqttTopic};
                          msg += F(":");
                          msg += String(reg.second.value);
                          msg += F(":");
                          msg += String(value);
                          msg += F(":");
                          msg += String(result);

                          auto path{topicbase};
                          path += F("last");

                          client.publish(path.c_str(), msg.c_str());
                      }
                  });
}

void myMqtt::reconnect()
{
    while (!client.connected())
    {
        Serial.println(F("Mqtt reconnect"));
        String clientId(F("esp8266-"));
        clientId += String(WiFi.macAddress());

#ifdef MQTTUSER
        if (client.connect(clientId.c_str(), MQTTUSER, MQTTPASS))
        {
            String path{mqtt_modbus_set_holdingregister};
            path += "#";

            client.subscribe(path.c_str());
        }
        else
        {
            delay(2000);
        }
#else
        if (client.connect(clientId.c_str()))
        {
            client.subscribe(mqtt_base_topic_subscribe);
        }
        else
        {
            delay(2000);
        }
#endif
    }
}

// Connect to WLAN subroutine
void myMqtt::connect()
{
    Serial.print(F("IP assigned by DHCP is "));
    Serial.println(WiFi.localIP());

    // connecting Mqtt
    Serial.println(F("Connecting to mqtt"));
    client.setServer(mqttBrokerIP, mqttBrokerPort);
    client.setCallback([this](char *topic, byte *payload, unsigned int length) { mqtt_callback(topic, payload, length); });

#ifdef MQTTUSER
    while (!client.connected())
    {
        String clientId(F("esp8266-"));
        clientId += String(WiFi.macAddress());

        if (client.connect(clientId.c_str(), MQTTUSER, MQTTPASS))
        {
            String path{mqtt_modbus_set_holdingregister};
            path += "#";

            client.subscribe(path.c_str());
        }
        else
        {
            delay(2000);
        }
    }
#else
    while (!client.connected())
    {
        String clientId(F("esp8266-"));
        clientId += String(WiFi.macAddress());
        if (client.connect(clientId.c_str()))
        {
            client.subscribe(mqtt_base_topic_subscribe);
        }
        else
        {
            delay(2000);
        }
    }
#endif
}
