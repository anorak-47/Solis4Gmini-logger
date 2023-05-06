#include "modbus.h"
#include "config.h"
#include <Arduino.h>
#include <ESPDash.h>

namespace
{
constexpr auto maxModbusReadRetries{3};
}

void dumpCard(UIDescription const &card)
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

    int irsPos{0};
    int regPos{0};

    memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));

    while (irs.registers)
    {
        regPos = 0;
        memcpy_P(&reg, &irs.registers[regPos], sizeof(ModbusRegisterDescription));

        while (reg.address != 0)
        {
            registerValues.emplace(reg.address, ModbusRegisterValue{0.0, false, &irs.registers[regPos]});

            ++regPos;
            memcpy_P(&reg, &irs.registers[regPos], sizeof(ModbusRegisterDescription));
        }

        memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));
    }
}

void ModbusSlaveDevice::resetRegisters(DeviceRegisterSet const *regSet)
{
    DeviceRegisterSet irs;
    ModbusRegisterDescription reg;

    int irsPos{0};
    int regPos{0};

    memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));

    while (irs.registers)
    {
        regPos = 0;
        memcpy_P(&reg, &irs.registers[regPos++], sizeof(ModbusRegisterDescription));

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

    int irsPos{0};
    int regPos{0};

    memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));

    while (irs.registers)
    {
        regPos = 0;
        memcpy_P(&reg, &irs.registers[regPos++], sizeof(ModbusRegisterDescription));

        while (reg.address != 0)
        {
            registerValues[reg.address].valid = false;

            memcpy_P(&reg, &irs.registers[regPos++], sizeof(ModbusRegisterDescription));
        }

        memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));
    }
}

float toFloat(uint16_t reg1, uint16_t reg2)
{
    /*
        float res = NAN;
        ((uint8_t *)&res)[3] = reg1 >> 8;
        ((uint8_t *)&res)[2] = reg1 & 0xff;
        ((uint8_t *)&res)[1] = reg2 >> 8;
        ((uint8_t *)&res)[0] = reg2 & 0xff;
        return res;
     */

    union
    {
        uint32_t x;
        float f;
    } res;

    res.x = ((static_cast<uint32_t>(reg1) << 16) | reg2);
    return res.f;
}

byte bcdToDec(byte val)
{
    return ((val / 16 * 10) + (val % 16));
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
    {
        auto buf{node->getResponseBuffer(0)};

        if (buf & (1 << offset))
            value = 1.0;
        break;
    }
    case ModbusRegisterValueType::Float:
        value = toFloat(node->getResponseBuffer(offset), node->getResponseBuffer(offset + 1));
        break;
    case ModbusRegisterValueType::BCD:
        value = bcdToDec(static_cast<int8_t>(node->getResponseBuffer(offset)));
        break;
    }

    return value * scale;
}

/* Modbus function 0x06 Write Single Register. */
uint8_t ModbusSlaveDevice::writeHoldingRegister(ModbusRegisterDescription const &reg, uint16_t value)
{
    auto u16WriteAddress{reg.address - getAddressOffset()};

    uint8_t result{};
    int retries{maxModbusReadRetries};
    bool timeout{true};

    while (retries > 0 && timeout)
    {
        result = rs485if->client()->writeSingleRegister(u16WriteAddress, value);
        timeout = result == ModbusMaster::ku8MBResponseTimedOut;
        --retries;

        if (retries > 0 && timeout)
            delay(MODBUS_WRITE_DELAY);
    }

    return result;
}

uint8_t ModbusSlaveDevice::readRegisters(uint16_t startAddress, uint8_t u16ReadQty)
{
    digitalWrite(2, LOW);

    auto u16ReadAddress{startAddress - getAddressOffset()};
    uint8_t result{};

    switch (getRegisterType())
    {
    case RegisterType::Coils:
        result = rs485if->client()->readCoils(u16ReadAddress, u16ReadQty);
        break;
    case RegisterType::DiscreteInputs:
        result = rs485if->client()->readDiscreteInputs(u16ReadAddress, u16ReadQty);
        break;
    case RegisterType::HoldingRegisters:
        result = rs485if->client()->readHoldingRegisters(u16ReadAddress, u16ReadQty);
        break;
    case RegisterType::InputRegisters:
        result = rs485if->client()->readInputRegisters(u16ReadAddress, u16ReadQty);
        break;
    }

    digitalWrite(2, HIGH);

    return result;
}

bool ModbusSlaveDevice::readRegisterSet(DeviceRegisterSet const *regSet)
{
    lastResult = result;

    DeviceRegisterSet irs;
    ModbusRegisterDescription reg;

    int irsPos{0};
    int regPos{0};

    int retries{maxModbusReadRetries};
    bool success{true};
    bool timeout{true};

    memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));

    while (irs.registers && success)
    {
        retries = maxModbusReadRetries;
        timeout = true;

        while (retries > 0 && timeout)
        {
            result = readRegisters(irs.startAddress, irs.registerCount);

            success = result == ModbusMaster::ku8MBSuccess;
            timeout = result == ModbusMaster::ku8MBResponseTimedOut;
            --retries;

            if (retries > 0 && timeout)
                delay(MODBUS_READ_DELAY);
        }

        retriesNeeded = maxModbusReadRetries - retries;

        if (success)
        {
            regPos = 0;
            memcpy_P(&reg, &irs.registers[regPos++], sizeof(ModbusRegisterDescription));

            while (reg.address != 0)
            {
                auto offset{reg.address - irs.startAddress};
                registerValues[reg.address].value = regToValue(rs485if->client(), offset, reg.type, reg.scale);
                registerValues[reg.address].valid = true;

                memcpy_P(&reg, &irs.registers[regPos++], sizeof(ModbusRegisterDescription));
            }
        }
        else
        {
            ++errorCount;
        }

        memcpy_P(&irs, &regSet[irsPos++], sizeof(DeviceRegisterSet));

        if (irs.registers && success)
            delay(MODBUS_READ_DELAY);
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
                      memcpy_P(&regDesc, reg.second.desc, sizeof(ModbusRegisterDescription));
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

uint8_t ModbusSlaveDevice::getServerId() const
{
    return rs485if->getServerId();
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
        Serial.println(F("no dev"));
    }

    return reachable;
}

bool ModbusSlaveDevice::hasErrorCodeChanged() const
{
    return lastResult != result;
}

uint32_t ModbusSlaveDevice::getErrorCount() const
{
    return errorCount;
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

ModbusSlaveDevice::ModbusSlaveDevice(RS485Interface &iface)
{
    rs485if = &iface;
}

String ModbusSlaveDevice::getName() const
{
    return {F("dev")};
}

String ModbusSlaveDevice::getMqttTopic() const
{
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

RS485Interface::RS485Interface(uint8_t slaveid, Stream &stream) : slaveid(slaveid)
{
    serial = &stream;
}

uint8_t RS485Interface::getServerId() const
{
    return slaveid;
}

void RS485Interface::begin()
{
    pinMode(MAX485_DE, OUTPUT);
    // Init in receive mode
    digitalWrite(MAX485_DE, 0);

    node.begin(slaveid, *serial); // SlaveID, Serial
    // Callbacks allow us to configure the RS485 transceiver correctly
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
}

ModbusMaster *RS485Interface::client()
{
    return &node;
}
