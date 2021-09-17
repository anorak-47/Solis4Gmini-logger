#pragma once
#include <SoftwareSerial.h>
#include <ModbusMaster.h>
#include <map>
#include <cinttypes>

enum class ModbusRegisterValueType
{
    U16,
    U32,
    S16,
    S32,
	DI
};

enum class CardType
{
	NoCard,
	Generic,
	Status,
	StatusInverted,
	Temperature
};

struct CardDescription
{
    const char *name;
    const char *unit;
    CardType type{CardType::NoCard};
};

struct ModbusRegisterDescription
{
    uint16_t address{};
    ModbusRegisterValueType type{ModbusRegisterValueType::U16};
    double scale{1.0};
    const char *mqttTopic{};
    CardDescription card{};
};


struct ModbusRegisterValue
{
    double value;
    bool valid;
    ModbusRegisterDescription const *reg{};
};

using RegisterValues = std::map<uint16_t, ModbusRegisterValue>;

struct DeviceRegisterSet
{
    uint16_t startAddress{};
    uint16_t registerCount{};
    ModbusRegisterDescription const *registers{};
};

class RS485Interface
{
public:
	void begin(uint8_t slaveid);
	ModbusMaster *master();

private:
    static void preTransmission();
    static void postTransmission();

    static SoftwareSerial serial;
	ModbusMaster node;
};

class ModbusSlaveDevice
{
public:
	enum class RegisterType
	{
		Coils,
		DiscreteInputs,
		HoldingRegisters,
		InputRegisters
	};

	virtual ~ModbusSlaveDevice() = default;

    void begin(RS485Interface &iface);
    bool request();
    
    bool isReachable() const;
    uint8_t getLastErrorCode() const;
    uint8_t getRetriesNeeded() const;

    virtual String getName() const;
    virtual String getMqttTopic() const;
    virtual RegisterType getRegisterType() const;
    virtual uint16_t getAddressOffset() const;

    void dumpRegisterValues() const;

    RegisterValues const &getRegisterValues() const;

    uint8_t writeHoldingRegister(ModbusRegisterDescription const &reg, uint16_t value);

protected:
    void initRegisters(DeviceRegisterSet const *regSet);
    void resetRegisters(DeviceRegisterSet const *regSet);
    void invalidateRegisters(DeviceRegisterSet const *regSet);
    bool readRegisterSet(DeviceRegisterSet const *regSet);
    uint8_t readRegisters(uint16_t u16ReadAddress, uint8_t u16ReadQty);

    const DeviceRegisterSet *deviceRegisters{};
    RegisterValues registerValues;
    RS485Interface *rs485if{};
    bool reachable{};
    uint8_t result{};
    uint8_t retriesNeeded{};
};
