#pragma once
#include "modbus.h"

class EnergyMonitorModbusSlaveDevice : public ModbusSlaveDevice
{
public:
    explicit EnergyMonitorModbusSlaveDevice(String const& part, RS485Interface &iface);
    uint16_t getAddressOffset() const override;
    String getMqttTopic() const override;

private:
    String mqttPathPart;
};

class EnergyMonitorInputValuesPower : public EnergyMonitorModbusSlaveDevice
{
public:
    explicit EnergyMonitorInputValuesPower(String const& part, RS485Interface &iface);
    String getName() const override;
};

class EnergyMonitorInputValues : public EnergyMonitorModbusSlaveDevice
{
public:
	explicit EnergyMonitorInputValues(String const& part, RS485Interface &iface);
	String getName() const override;
};

class EnergyMonitorHoldingRegisters : public EnergyMonitorModbusSlaveDevice
{
public:
	explicit EnergyMonitorHoldingRegisters(String const& part, RS485Interface &iface);
	String getName() const override;
	RegisterType getRegisterType() const override;
};
