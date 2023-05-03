#pragma once

#include "modbus.h"
#include <espMqttClientAsync.h>
#include <Arduino.h>

class MQTTClient
{
public:
    void begin();
    void loop();

    void sendStatus();
    void sendPayload(String const &path, String const &payload);

    void addModbusSlave(ModbusSlaveDevice &device);
    void subscribeAll() const;

private:
    void connect();

    void onMqttConnect(bool sessionPresent);
    void onMqttDisconnect(espMqttClientTypes::DisconnectReason reason);
    void onMqttSubscribe(uint16_t packetId, const espMqttClientTypes::SubscribeReturncode *codes, size_t len);
    void onMqttUnsubscribe(uint16_t packetId);
    void onMqttPublish(uint16_t packetId);
    void onMqttMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic, const uint8_t *payload, size_t len, size_t index,
                       size_t total);

    void writePayload(String const &sTopic, String const &payloadStr) const;
    void writePayload(ModbusSlaveDevice *modbusSlave, String const &sTopic, String const &payloadStr) const;
    void setRelay(String const &sTopic, String const &payloadStr) const;
    uint8_t writeRegister(ModbusSlaveDevice *modbusSlave, ModbusRegisterDescription const &regDesc, uint16_t value) const;
    uint16_t getValue(ModbusRegisterDescription const &regDesc, String const &payloadStr) const;
    void subscribe(String const &topic, uint8_t qos = 0) const;

    std::vector<ModbusSlaveDevice *> modbusSlaves;
};
