#ifndef MQTT_H
#define MQTT_H

#include "main.h"

class MQTTClient
{
public:
    void begin();
    void loop();
    void sendStatus();
    void sendPayload(String const &path, String const &payload);

    void addModbusSlave(ModbusSlaveDevice &device);
    void subscribe() const;

private:
    void reconnect();
    void connect();
    void mqtt_callback(char *topic, byte *payload, unsigned int length);
    void writePayload(String const &sTopic, String const &payloadStr) const;
    void writePayload(ModbusSlaveDevice *modbusSlave, String const &sTopic, String const &payloadStr) const;
    void setRelay(String const &sTopic, String const &payloadStr) const;
    uint8_t writeRegister(ModbusSlaveDevice *modbusSlave, ModbusRegisterDescription const &regDesc, uint16_t value) const;
    uint16_t getValue(ModbusRegisterDescription const &regDesc, String const &payloadStr) const;
    void subscribe(String const& topic) const;

    std::vector<ModbusSlaveDevice *> modbusSlaves;
};

#endif
