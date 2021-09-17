#pragma once
#include "modbus.h"

class InverterInputValuesPower : public ModbusSlaveDevice
{
public:
    InverterInputValuesPower();
    String getName() const override;
};

class InverterInputValues : public ModbusSlaveDevice
{
public:
	InverterInputValues();
	String getName() const override;
};

class InverterHoldingRegisters : public ModbusSlaveDevice
{
public:
	InverterHoldingRegisters();
	String getName() const override;
	RegisterType getRegisterType() const override;
};

class InverterDiscreteInputs : public ModbusSlaveDevice
{
public:
	InverterDiscreteInputs();
	String getName() const override;
	RegisterType getRegisterType() const override;
	uint16_t getAddressOffset() const override;
};
