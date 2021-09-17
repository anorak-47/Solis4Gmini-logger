#ifndef MQTT_H
#define MQTT_H

#include "main.h"

class myMqtt
{
public:
    myMqtt();

    void begin();
    void loop();
    void sendStatus();
    void sendPayload(String const &path, String const& payload);

    void setModbusSlave(ModbusSlaveDevice &device);

private:
    void reconnect();
    void connect();
    void mqtt_callback(char *topic, byte *payload, unsigned int length);

    ModbusSlaveDevice *modbusSlave{};
};

#endif
