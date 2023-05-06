#include "mqtt.h"

#ifndef ESPMQTT

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

namespace
{
constexpr unsigned long mqttTryReconnectInterval{30000};

Relay relay;
WiFiClient net;
PubSubClient client(net);
} // namespace

void MQTTClient::begin()
{
    // net.setTimeout(MQTT_WIFI_CLIENT_TIMEOUT * 100);
    // client.setKeepAlive(MQTT_KEEPALIVE);
    // client.setSocketTimeout(MQTT_SOCKET_TIMEOUT);

    client.setBufferSize(512);
    client.setServer(MQTT_HOST, MQTT_PORT);
    client.setCallback([this](char *topic, byte *payload, unsigned int length) { mqtt_callback(topic, payload, length); });

    relay.begin();
    connect();
}

void MQTTClient::loop()
{
    if (!client.loop())
    {
        if (millis() - lastReconnectTry > mqttTryReconnectInterval)
        {
            connect();
        }
    }
}

void MQTTClient::connect()
{
    Serial.println(F("Connecting to MQTT"));

    if (!client.connected())
    {
        Serial.println(F("MQTT connect"));

        if (client.connect(HOSTNAME, MQTT_USER, MQTT_PASSWORD, mqtt_status_control_topic, 2, true, "offline"))
        {
            subscribe();
            
            client.publish(mqtt_status_control_topic, "online", true);            
            client.publish(mqtt_status_reconnect_topic, String(++reconnectCounter).c_str(), true);
        }
        else
        {
            lastReconnectTry = millis();
        }

        Serial.println(F("connected to MQTT"));
    }
}

bool MQTTClient::isConnected() const
{
	return client.connected();
}

void MQTTClient::sendPayload(String const &path, String const &payload)
{
    client.publish(path.c_str(), payload.c_str());
}

void MQTTClient::addModbusSlave(ModbusSlaveDevice &device)
{
    modbusSlaves.emplace_back(&device);
}

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
                      memcpy_P(&regDesc, reg.second.desc, sizeof(ModbusRegisterDescription));
                      auto mqttTopic{String{regDesc.mqttTopic}};

                      if (topicbase + mqttTopic == sTopic)
                      {
                          auto value{getValue(regDesc, payloadStr)};
                          auto result{writeRegister(modbusSlave, regDesc, value)};

                          auto payload{String(reg.second.value)};
                          payload += F(":");
                          payload += String(value);
                          payload += F(":");
                          payload += String(result);

                          String resultTopic{modbusSlave->getMqttTopic() + modbusSlave->getName() + F(mqtt_modbus_set_result_holdingregister) +
                                             mqttTopic};

                          client.publish(resultTopic.c_str(), payload.c_str());
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

#else

#include <ESP8266WiFi.h>

//
// https://www.emelis.net/espMqttClient/
//

namespace
{
constexpr unsigned long mqttTryReconnectInterval{30000};
} // namespace

MQTTClient::MQTTMessage::MQTTMessage(String const &t, String const &p) : topic{t}, payload{p}
{
}

MQTTClient::MQTTMessage::MQTTMessage(char const *t, char const *p) : topic{t}, payload{p}
{
}

void MQTTClient::begin()
{
#ifdef DEVICE_RELAY
    relay.begin();
#endif

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setClientId(HOSTNAME);
    mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);
    mqttClient.setWill(mqtt_status_control_topic, 2, true, "offline");

    mqttClient.onConnect([this](bool sessionPresent) { onMqttConnect(sessionPresent); });
    mqttClient.onDisconnect([this](espMqttClientTypes::DisconnectReason reason) { onMqttDisconnect(reason); });

    // mqttClient.onSubscribe(onMqttSubscribe);
    // mqttClient.onUnsubscribe(onMqttUnsubscribe);

    // mqttClient.onPublish(onMqttPublish);
    mqttClient.onMessage([this](const espMqttClientTypes::MessageProperties &properties, const char *topic, const uint8_t *payload, size_t len,
                                size_t index, size_t total) { onMqttMessage(properties, topic, payload, len, index, total); });

    connect();
}

void MQTTClient::loop()
{
    if (reconnectMqtt)
    {
        if (millis() - lastReconnectTry > mqttTryReconnectInterval)
        {
            connect();
        }
    }

    mqttClient.loop();
    handleMessages();
}

// Connect to WLAN subroutine
void MQTTClient::connect()
{
    Serial.println("connecting to MQTT...");
    if (!mqttClient.connect())
    {
        reconnectMqtt = true;
        lastReconnectTry = millis();
    }
    else
    {
        reconnectMqtt = false;
    }
}

void MQTTClient::onMqttConnect(bool sessionPresent)
{
    Serial.println("connected to MQTT");

    subscribeAll();
    mqttClient.publish(mqtt_status_control_topic, 2, true, "online");
}

void MQTTClient::onMqttDisconnect(espMqttClientTypes::DisconnectReason reason)
{
    Serial.printf("disconnected from MQTT: %u\n", static_cast<uint8_t>(reason));

    if (WiFi.isConnected())
    {
        reconnectMqtt = true;
        lastReconnectTry = millis();
    }
}

void MQTTClient::onMqttSubscribe(uint16_t packetId, const espMqttClientTypes::SubscribeReturncode *codes, size_t len)
{
    Serial.println("Subscribe acknowledged");
    Serial.print("  packetId: ");
    Serial.println(packetId);
    for (size_t i = 0; i < len; ++i)
    {
        Serial.print("  qos: ");
        Serial.println(static_cast<uint8_t>(codes[i]));
    }
}

void MQTTClient::onMqttUnsubscribe(uint16_t packetId)
{
    Serial.println("Unsubscribe acknowledged");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}

void MQTTClient::onMqttMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic, const uint8_t *payload, size_t len,
                               size_t index, size_t total)
{
    constexpr auto maxPlayloadLength{100};
    static char strval[maxPlayloadLength + 1];

    if (len > maxPlayloadLength || len == 0)
        return;

    memcpy(strval, payload, len);
    strval[len] = '\0';

    popLocked = true;

    while (pushLocked)
        delay(1);

    messages.emplace_back(topic, strval);

    dataReady = true;
    popLocked = false;
}

MQTTClient::MQTTMessage MQTTClient::popMsg()
{
    if (popLocked)
        return {};

    pushLocked = true;

    auto result{messages.back()};
    messages.pop_back();

    if (messages.empty())
        dataReady = false;

    pushLocked = false;

    return result;
}

void MQTTClient::handleMessages()
{
    if (dataReady)
    {
        auto msg{popMsg()};

#ifdef DEVICE_RELAY
        if (msg.topic == F(mqtt_relay_set_topic))
        {
            setRelay(msg.topic, msg.payload);
            return;
        }
#endif

        writePayload(msg.topic, msg.payload);
    }
}

void MQTTClient::onMqttPublish(uint16_t packetId)
{
    Serial.println("Publish acknowledged");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}

void MQTTClient::sendPayload(String const &path, String const &payload)
{
    mqttClient.publish(path.c_str(), 0, false, payload.c_str());
}

void MQTTClient::addModbusSlave(ModbusSlaveDevice &device)
{
    modbusSlaves.emplace_back(&device);
}

void MQTTClient::sendStatus()
{
    static char bufferSend[10];
    dtostrf(WiFi.RSSI(), 0, 0, bufferSend);
    mqttClient.publish(mqtt_status_rssi_topic, 0, false, bufferSend);
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

void MQTTClient::writePayload(ModbusSlaveDevice *modbusSlave, String const &sTopic, String const &payloadStr)
{
    auto registers{modbusSlave->getRegisterValues()};

    ModbusRegisterDescription regDesc;
    String topicbase{modbusSlave->getMqttTopic() + modbusSlave->getName() + F(mqtt_modbus_set_holdingregister)};

    std::for_each(std::begin(registers), std::end(registers),
                  [&modbusSlave, &topicbase, &sTopic, &regDesc, &payloadStr, this](auto const &reg)
                  {
                      memcpy_P(&regDesc, reg.second.desc, sizeof(ModbusRegisterDescription));
                      auto mqttTopic{String{regDesc.mqttTopic}};

                      if (topicbase + mqttTopic == sTopic)
                      {
                          auto value{getValue(regDesc, payloadStr)};
                          auto result{writeRegister(modbusSlave, regDesc, value)};

                          auto payload{String(reg.second.value)};
                          payload += F(":");
                          payload += String(value);
                          payload += F(":");
                          payload += String(result);

                          String resultTopic{modbusSlave->getMqttTopic() + modbusSlave->getName() + F(mqtt_modbus_set_result_holdingregister) +
                                             mqttTopic};
                          mqttClient.publish(resultTopic.c_str(), 0, false, payload.c_str());
                      }
                  });
}

void MQTTClient::writePayload(String const &sTopic, String const &payloadStr)
{
    if (sTopic.isEmpty() || payloadStr.isEmpty())
        return;

    std::for_each(std::begin(modbusSlaves), std::end(modbusSlaves),
                  [&sTopic, &payloadStr, this](auto const &modbusSlave)
                  {
                      String setHRTopic{modbusSlave->getMqttTopic() + modbusSlave->getName() + F(mqtt_modbus_set_holdingregister)};
                      if (sTopic.startsWith(setHRTopic))
                          writePayload(modbusSlave, sTopic, payloadStr);
                  });
}

void MQTTClient::setRelay(String const &sTopic, String const &payloadStr)
{
#ifdef DEVICE_RELAY
    if (sTopic.isEmpty() || payloadStr.isEmpty())
        return;

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
    mqttClient.publish(stateTopic.c_str(), 0, false, valueStr.c_str());
#endif
}

void MQTTClient::subscribe(String const &topic, uint8_t qos)
{
    // Serial.print(F("subscribe to: "));
    // Serial.println(topic);

    mqttClient.subscribe(topic.c_str(), qos);
}

void MQTTClient::subscribeAll()
{
    std::for_each(std::begin(modbusSlaves), std::end(modbusSlaves),
                  [this](auto const &modbusSlave)
                  {
                      String topic{modbusSlave->getMqttTopic() + modbusSlave->getName() + F(mqtt_modbus_set_holdingregister) + F("#")};
                      subscribe(topic);
                  });

#ifdef DEVICE_RELAY
    auto relayTopic{String(F(mqtt_relay_set_topic))};
    subscribe(relayTopic, 2);
#endif
}

#endif
