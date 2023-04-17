#include "mqtt.h"
#include "relay.h"

Relay relay;

WiFiClient net;
PubSubClient client(net);

/*
Starts MQTT
*/
void MQTTClient::begin()
{
    net.setTimeout(MQTT_WIFI_CLIENT_TIMEOUT * 100);
    client.setKeepAlive(MQTT_KEEPALIVE);
    client.setSocketTimeout(MQTT_SOCKET_TIMEOUT);

    relay.begin();
    connect();
}

/*
Loops MQTT
*/
void MQTTClient::loop()
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
void MQTTClient::sendPayload(String const &path, String const &payload)
{
    client.publish(path.c_str(), payload.c_str());
}

void MQTTClient::addModbusSlave(ModbusSlaveDevice &device)
{
    modbusSlaves.emplace_back(&device);
}

/*
Sends status
*/
void MQTTClient::sendStatus()
{
    char bufferSend[10];
    dtostrf(WiFi.RSSI(), 0, 0, bufferSend);
    client.publish(mqtt_status_rssi_topic, bufferSend);
}

uint8_t decToBcd(uint8_t val)
{
    return (((val / 10) * 16) + (val % 10));
}

uint16_t MQTTClient::getValue(ModbusRegisterDescription const &regDesc, String const &payloadStr) const
{
    auto value{strtol(payloadStr.c_str(), nullptr, 10)};

    if (regDesc.type == ModbusRegisterValueType::BCD)
    {
        return decToBcd(static_cast<uint8_t>(value));
    }

    if (regDesc.scale != 1.0)
    {
        value = static_cast<uint16_t>(static_cast<double>(value) / regDesc.scale);
    }

    return value;
}

uint8_t MQTTClient::writeRegister(ModbusSlaveDevice *modbusSlave, ModbusRegisterDescription const &regDesc, uint16_t value) const
{
    return modbusSlave->writeHoldingRegister(regDesc, value);
}

void MQTTClient::writePayload(ModbusSlaveDevice *modbusSlave, String const &sTopic, String const &payloadStr) const
{
    auto registers{modbusSlave->getRegisterValues()};

    ModbusRegisterDescription regDesc;
    String topicbase{modbusSlave->getMqttTopic() + modbusSlave->getName() + F(mqtt_modbus_set_holdingregister)};

    std::for_each(std::begin(registers), std::end(registers),
                  [&modbusSlave, &topicbase, &sTopic, &regDesc, &payloadStr, this](auto const &reg)
                  {
                      memcpy_P(&regDesc, reg.second.reg, sizeof(ModbusRegisterDescription));
                      auto mqttTopic{String{regDesc.mqttTopic}};

                      if (topicbase + mqttTopic == sTopic)
                      {
                          auto value{getValue(regDesc, payloadStr)};
                          auto result{writeRegister(modbusSlave, regDesc, value)};

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

void MQTTClient::writePayload(String const &sTopic, String const &payloadStr) const
{
    std::for_each(std::begin(modbusSlaves), std::end(modbusSlaves),
                  [&sTopic, &payloadStr, this](auto const &modbusSlave)
                  {
                      String setHRTopic{modbusSlave->getMqttTopic() + modbusSlave->getName() + F(mqtt_modbus_set_holdingregister)};
                      Serial.println(setHRTopic);
                      if (sTopic.startsWith(setHRTopic))
                          writePayload(modbusSlave, sTopic, payloadStr);
                  });
}

void MQTTClient::setRelay(String const &sTopic, String const &payloadStr) const
{
#ifdef DEVICE_RELAY
    auto value{strtol(payloadStr.c_str(), nullptr, 10)};

    if (value > 0)
    {
        value = 1;
        relay.setOn();
    }
    else
    {
        value = 0;
        relay.setOff();
    }

    String stateTopic{F(mqtt_relay_state_topic)};
    String valueStr(value);
    client.publish(stateTopic.c_str(), valueStr.c_str());
#endif
}

void MQTTClient::mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    String sTopic{topic};

#ifdef DEVICE_RELAY
    if (sTopic == F(mqtt_relay_set_topic))
    {
        payload[length] = '\0';
        String payloadStr(reinterpret_cast<char *>(payload));
        setRelay(sTopic, payloadStr);
        return;
    }
#endif

    if (modbusSlaves.empty())
        return;

    payload[length] = '\0';
    String payloadStr(reinterpret_cast<char *>(payload));

    writePayload(sTopic, payloadStr);
}

void MQTTClient::subscribe(String const &topic) const
{
    // Serial.print(F("subscribe to: "));
    // Serial.println(topic);

    client.subscribe(topic.c_str());
}

void MQTTClient::subscribe() const
{
    std::for_each(std::begin(modbusSlaves), std::end(modbusSlaves),
                  [this](auto const &modbusSlave)
                  {
                      String topic{modbusSlave->getMqttTopic() + modbusSlave->getName() + F(mqtt_modbus_set_holdingregister) + F("#")};
                      subscribe(topic);
                  });

#ifdef DEVICE_RELAY
    auto topic{String(F(mqtt_relay_set_topic))};
    subscribe(topic);
#endif
}

void MQTTClient::reconnect()
{
    while (!client.connected())
    {
        Serial.println(F("MQTT reconnect"));
        String clientId(F("esp8266-"));
        clientId += String(WiFi.macAddress());

        if (client.connect(clientId.c_str(), MQTTUSER, MQTTPASS, mqtt_status_control_topic, 1, true, "offline"))
        {
            client.publish(mqtt_status_control_topic, "online", true);
            subscribe();
        }
        else
        {
            delay(2000);
        }
    }

    Serial.println(F("connected to MQTT"));
}

// Connect to WLAN subroutine
void MQTTClient::connect()
{
    Serial.print(F("IP assigned by DHCP is "));
    Serial.println(WiFi.localIP());

    // connecting Mqtt
    Serial.println(F("Connecting to MQTT"));
    client.setServer(mqttBrokerIP, mqttBrokerPort);
    client.setCallback([this](char *topic, byte *payload, unsigned int length) { mqtt_callback(topic, payload, length); });

    while (!client.connected())
    {
        Serial.println(F("MQTT reconnect"));
        String clientId(F("esp8266-"));
        clientId += String(WiFi.macAddress());

        if (!client.connect(clientId.c_str(), MQTTUSER, MQTTPASS, mqtt_status_control_topic, 1, true, "offline"))
        {
            delay(2000);
        }
        else
        {
            client.publish(mqtt_status_control_topic, "online", true);
        }

        Serial.println(F("connected to MQTT"));
    }
}
