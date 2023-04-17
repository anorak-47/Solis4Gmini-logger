#pragma once
#include "modbus.h"

class InverterInputValuesPower : public ModbusSlaveDevice
{
public:
    explicit InverterInputValuesPower(RS485Interface &iface);
    String getName() const override;
};

class InverterInputValues : public ModbusSlaveDevice
{
public:
	explicit InverterInputValues(RS485Interface &iface);
	String getName() const override;
};

class InverterHoldingRegisters : public ModbusSlaveDevice
{
public:
	explicit InverterHoldingRegisters(RS485Interface &iface);
	String getName() const override;
	RegisterType getRegisterType() const override;
};

class InverterDiscreteInputs : public ModbusSlaveDevice
{
public:
	explicit InverterDiscreteInputs(RS485Interface &iface);
	String getName() const override;
	RegisterType getRegisterType() const override;
	uint16_t getAddressOffset() const override;
};
