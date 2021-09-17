#include "modbus.h"
#include "config.h"
#include <Arduino.h>
#include <ESPDash.h>

namespace
{
constexpr auto maxModbusReadRetries{3};
}

void dumpCard(CardDescription const &card)
{
    Serial.println(F("---- card"));
    Serial.println(card.name);
    Serial.println(card.unit);
}

void dumpRegisters(DeviceRegisterSet const *regSet)
{
    Serial.println(F("modbus registers"));

    DeviceRegisterSet irs;
    ModbusRegisterDescription reg;

    int irsPos{};
    int regPos{};

    memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));

    while (irs.registers)
    {
        Serial.print(F("---- register set "));
        Serial.println(irsPos);
        Serial.print(F("start address: "));
        Serial.println(irs.startAddress);
        Serial.print(F("registers: "));
        Serial.println(irs.registerCount);
        Serial.println(F("----"));

        regPos = 0;
        memcpy_P(&reg, &irs.registers[regPos++], sizeof(ModbusRegisterDescription));

        while (reg.address != 0)
        {
            Serial.print(F("-- register "));
            Serial.println(regPos);
            Serial.print(F("address: "));
            Serial.println(reg.address);
            Serial.print(F("topic: "));
            Serial.println(reg.mqttTopic);
            dumpCard(reg.card);

            memcpy_P(&reg, &irs.registers[regPos++], sizeof(ModbusRegisterDescription));
        }

        memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));
    }
}

void ModbusSlaveDevice::initRegisters(DeviceRegisterSet const *regSet)
{
    DeviceRegisterSet irs;
    ModbusRegisterDescription reg;

    int irsPos{};
    int regPos{};

    memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));

    while (irs.registers)
    {
        regPos = 0;
        memcpy_P(&reg, &irs.registers[regPos], sizeof(ModbusRegisterDescription));

        while (reg.address != 0)
        {
            registerValues.emplace(reg.address, ModbusRegisterValue{0.0, false, &irs.registers[regPos]});

            regPos++;
            memcpy_P(&reg, &irs.registers[regPos], sizeof(ModbusRegisterDescription));
        }

        memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));
    }
}

void ModbusSlaveDevice::resetRegisters(DeviceRegisterSet const *regSet)
{
    DeviceRegisterSet irs;
    ModbusRegisterDescription reg;

    int irsPos{};
    int regPos{};

    memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));

    while (irs.registers)
    {
        regPos = 0;
        memcpy_P(&reg, &irs.registers[regPos], sizeof(ModbusRegisterDescription));

        while (reg.address != 0)
        {
            registerValues[reg.address].value = 0.0;
            registerValues[reg.address].valid = false;
            memcpy_P(&reg, &irs.registers[regPos++], sizeof(ModbusRegisterDescription));
        }

        memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));
    }
}

void ModbusSlaveDevice::invalidateRegisters(DeviceRegisterSet const *regSet)
{
    DeviceRegisterSet irs;
    ModbusRegisterDescription reg;

    int irsPos{};
    int regPos{};

    memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));

    while (irs.registers)
    {
        regPos = 0;
        memcpy_P(&reg, &irs.registers[regPos], sizeof(ModbusRegisterDescription));

        while (reg.address != 0)
        {
            registerValues[reg.address].valid = false;
            memcpy_P(&reg, &irs.registers[regPos++], sizeof(ModbusRegisterDescription));
        }

        memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));
    }
}

double regToValue(ModbusMaster *node, uint16_t offset, ModbusRegisterValueType type, double scale)
{
    double value{};

    switch (type)
    {
    case ModbusRegisterValueType::U16:
        value = node->getResponseBuffer(offset);
        break;
    case ModbusRegisterValueType::S16:
        value = static_cast<int16_t>(node->getResponseBuffer(offset));
        break;
    case ModbusRegisterValueType::U32:
        value = (static_cast<uint32_t>(node->getResponseBuffer(offset)) << 16) + static_cast<uint32_t>(node->getResponseBuffer(offset + 1));
        break;
    case ModbusRegisterValueType::S32:
        value = static_cast<int32_t>((static_cast<uint32_t>(node->getResponseBuffer(offset)) << 16) +
                                     static_cast<uint32_t>((node->getResponseBuffer(offset + 1))));
        break;
    case ModbusRegisterValueType::DI:
        auto buf{node->getResponseBuffer(0)};

        if (buf & (1 << offset))
            value = 1.0;

        break;
    }

    return value * scale;
}

/* Modbus function 0x06 Write Single Register. */
uint8_t ModbusSlaveDevice::writeHoldingRegister(ModbusRegisterDescription const &reg, uint16_t value)
{
	auto u16WriteAddress{reg.address - getAddressOffset()};
	return rs485if->master()->writeSingleRegister(u16WriteAddress, value);
}

uint8_t ModbusSlaveDevice::readRegisters(uint16_t startAddress, uint8_t u16ReadQty)
{
    digitalWrite(2, LOW);
    auto u16ReadAddress{startAddress - getAddressOffset()};
    uint8_t result{};

    switch (getRegisterType())
    {
    case RegisterType::Coils:
        result = rs485if->master()->readCoils(u16ReadAddress, u16ReadQty);
        break;
    case RegisterType::DiscreteInputs:
        result = rs485if->master()->readDiscreteInputs(u16ReadAddress, u16ReadQty);
        break;
    case RegisterType::HoldingRegisters:
        result = rs485if->master()->readHoldingRegisters(u16ReadAddress, u16ReadQty);
        break;
    case RegisterType::InputRegisters:
        result = rs485if->master()->readInputRegisters(u16ReadAddress, u16ReadQty);
        break;
    }

    digitalWrite(2, HIGH);

    return result;
}

bool ModbusSlaveDevice::readRegisterSet(DeviceRegisterSet const *regSet)
{
    DeviceRegisterSet irs;
    ModbusRegisterDescription reg;
    bool success{true};
    int irsPos{};
    int regPos{};

    memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));

    while (irs.registers && success)
    {
        int retries = maxModbusReadRetries;
        bool timeout{true};

        while (retries > 0 && timeout)
        {
            result = readRegisters(irs.startAddress, irs.registerCount);
            success = result == ModbusMaster::ku8MBSuccess;
            timeout = result == ModbusMaster::ku8MBResponseTimedOut;
            --retries;
            delay(readDelay / 2);
        }

        retriesNeeded = maxModbusReadRetries - retries;

        regPos = 0;
        memcpy_P(&reg, &irs.registers[regPos++], sizeof(ModbusRegisterDescription));

        while (reg.address != 0)
        {
            if (success)
            {
                auto offset{reg.address - irs.startAddress};
                registerValues[reg.address].value = regToValue(rs485if->master(), offset, reg.type, reg.scale);
                registerValues[reg.address].valid = true;
            }

            memcpy_P(&reg, &irs.registers[regPos++], sizeof(ModbusRegisterDescription));
        }

        memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));
        delay(readDelay);
    }

    return success;
}

void ModbusSlaveDevice::dumpRegisterValues() const
{
    Serial.print(F("register count: "));
    Serial.println(registerValues.size());

    ModbusRegisterDescription regDesc;
    std::for_each(std::begin(registerValues), std::end(registerValues),
                  [&regDesc](auto const &reg)
                  {
                      memcpy_P(&regDesc, reg.second.reg, sizeof(ModbusRegisterDescription));
                      Serial.print(F("register "));
                      Serial.print(regDesc.address);
                      Serial.print(F(", valid: "));
                      Serial.print(reg.second.valid);
                      Serial.print(F(", value: "));
                      Serial.println(reg.second.value);
                  });
}

RegisterValues const &ModbusSlaveDevice::getRegisterValues() const
{
    return registerValues;
}

/*
Requests data from the inverter
*/
bool ModbusSlaveDevice::request()
{
    // dumpRegisters(deviceRegisters);
    reachable = readRegisterSet(deviceRegisters);
    // dumpRegisterValues();

    if (!reachable)
    {
        // resetRegisters(deviceRegisters);
        invalidateRegisters(deviceRegisters);
        Serial.println(F("Inverter not reachable"));
    }

    return reachable;
}

/*
Is device reachable
*/
bool ModbusSlaveDevice::isReachable() const
{
    return reachable;
}

uint8_t ModbusSlaveDevice::getLastErrorCode() const
{
    return result;
}

uint8_t ModbusSlaveDevice::getRetriesNeeded() const
{
    return retriesNeeded;
}

void ModbusSlaveDevice::begin(RS485Interface &iface)
{
    rs485if = &iface;
}

String ModbusSlaveDevice::getName() const
{
    return {F("dev")};
}

String ModbusSlaveDevice::getMqttTopic() const
{
    // return String(F(mqtt_base_topic)) + F("SENSOR");
    return String(F(mqtt_base_topic));
}

ModbusSlaveDevice::RegisterType ModbusSlaveDevice::getRegisterType() const
{
    return RegisterType::InputRegisters;
}

uint16_t ModbusSlaveDevice::getAddressOffset() const
{
    return 1;
}

/////////////////////////////////////////////////////

void RS485Interface::preTransmission()
{
    digitalWrite(MAX485_DE, 1);
}

void RS485Interface::postTransmission()
{
    digitalWrite(MAX485_DE, 0);
}

SoftwareSerial RS485Interface::serial(rs485_RX, rs485_TX, false); // RX, TX, Invert signal

void RS485Interface::begin(uint8_t slaveid)
{
    serial.begin(9600);

    pinMode(MAX485_DE, OUTPUT);
    // Init in receive mode
    digitalWrite(MAX485_DE, 0);

    node.begin(slaveid, serial); // SlaveID, Serial
    // Callbacks allow us to configure the RS485 transceiver correctly
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
}

ModbusMaster *RS485Interface::master()
{
    return &node;
}
