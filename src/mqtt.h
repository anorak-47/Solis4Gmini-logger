#pragma once
#include "config.h"
#include "relay.h"
#include "modbus.h"

#ifndef ESPMQTT

class MQTTClient
{
public:
    void begin();
    void loop();
    
    bool isConnected() const;
    
    void sendStatus();
    void sendPayload(String const &topic, String const &payload) const;
    void sendPayload(char const *topic, char const *payload) const;

    void addModbusSlave(ModbusSlaveDevice &device);
    void subscribe() const;

private:
    void connect();
    
    void mqtt_callback(char *topic, byte *payload, unsigned int length);
    void writePayload(String const &sTopic, String const &payloadStr) const;
    void writePayload(ModbusSlaveDevice *modbusSlave, String const &sTopic, String const &payloadStr) const;
    void setRelay(String const &sTopic, String const &payloadStr) const;
    uint8_t writeRegister(ModbusSlaveDevice *modbusSlave, ModbusRegisterDescription const &regDesc, uint16_t value) const;
    uint16_t getValue(ModbusRegisterDescription const &regDesc, String const &payloadStr) const;
    void subscribe(String const& topic) const;

    std::vector<ModbusSlaveDevice *> modbusSlaves;
        
    uint32_t lastReconnectTry{0};
    uint32_t reconnectCounter{0};
};

#else

#include <espMqttClient.h>
#include <Arduino.h>

class MQTTClient
{
public:
    struct MQTTMessage
    {
        MQTTMessage() = default;
        MQTTMessage(String const &t, String const &p);
        MQTTMessage(char const *t, char const *p);

        String topic;
        String payload;
    };

    using MsgQueue = std::vector<MQTTMessage>;

    void begin();
    void loop();
    
    bool isConnected() const;

    void sendStatus();
    void sendPayload(String const &topic, String const &payload);
    void sendPayload(char const *topic, char const *payload);

    void addModbusSlave(ModbusSlaveDevice &device);

private:
    void connect();

    MQTTMessage popMsg();
    void handleMessages();

    void onMqttConnect(bool sessionPresent);
    void onMqttDisconnect(espMqttClientTypes::DisconnectReason reason);
    void onMqttSubscribe(uint16_t packetId, const espMqttClientTypes::SubscribeReturncode *codes, size_t len);
    void onMqttUnsubscribe(uint16_t packetId);
    void onMqttPublish(uint16_t packetId);
    void onMqttMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic, const uint8_t *payload, size_t len, size_t index,
                       size_t total);

    void writePayload(String const &sTopic, String const &payloadStr);
    void writePayload(ModbusSlaveDevice *modbusSlave, String const &sTopic, String const &payloadStr);
    void setRelay(String const &sTopic, String const &payloadStr);
    uint8_t writeRegister(ModbusSlaveDevice *modbusSlave, ModbusRegisterDescription const &regDesc, uint16_t value) const;
    uint16_t getValue(ModbusRegisterDescription const &regDesc, String const &payloadStr) const;
    void subscribe(String const &topic, uint8_t qos = 0);
    void subscribeAll();

    std::vector<ModbusSlaveDevice *> modbusSlaves;

    MsgQueue messages;
    std::atomic_bool popLocked{false};
    std::atomic_bool pushLocked{false};
    std::atomic_bool dataReady{false};

    bool reconnectMqtt{false};
    uint32_t lastReconnectTry{0};

    espMqttClient mqttClient;

#ifdef DEVICE_RELAY
    Relay relay;
#endif
};

#endif
