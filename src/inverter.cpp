#include "inverter.h"
#include "main.h"

// clang-format off

/* ----------------- Solis 4G / 5G Power Input Registers ---------------- */

const ModbusRegisterDescription regF1[3] PROGMEM =
{
	{3005, ModbusRegisterValueType::U32, 1.0, "3005", {"Power", "W", CardType::Generic}},
	{3007, ModbusRegisterValueType::U32, 1.0, "3007", {"DC out", "W", CardType::NoCard}},
	{}
};

const ModbusRegisterDescription regF2[3] PROGMEM =
{
	{3056, ModbusRegisterValueType::S32, 1.0, "3056", {"Reactive power", "var", CardType::NoCard}},
	{3058, ModbusRegisterValueType::S32, 1.0, "3058", {"Apparent power", "VA", CardType::Generic}},
	{}
};

const ModbusRegisterDescription regF3[4] PROGMEM =
{
	{3088, ModbusRegisterValueType::U16, 1.0, "3088", {"Power limit op", "", CardType::NoCard}},
	{3089, ModbusRegisterValueType::U16, 1.0, "3089", {"Reac power limit op", "", CardType::NoCard}},
	{3090, ModbusRegisterValueType::U16, 1.0, "3090", {"Power limit sw", "", CardType::NoCard}},
	{}
};

const DeviceRegisterSet inverterRegistersPower[3] PROGMEM =
{
	{3005, 4, regF1},
	{3056, 4, regF2},
	/*{3088, 3, regF3},*/
	{}
};

/* ----------------- Solis 4G / 5G Other Input Registers ---------------- */

const ModbusRegisterDescription regS1[6] PROGMEM =
{
	{3009, ModbusRegisterValueType::U32, 1.0, "3009", {"Total energy", "kWh", CardType::Generic}},
	{3011, ModbusRegisterValueType::U32, 1.0, "3011", {"Energy this month", "kWh", CardType::Generic}},
	{3013, ModbusRegisterValueType::U32, 1.0, "3013", {"Engery last month", "kWh", CardType::NoCard}},
	{3015, ModbusRegisterValueType::U16, 0.1, "3015", {"Energy this day", "kWh", CardType::Generic}},
	{3016, ModbusRegisterValueType::U16, 0.1, "3016", {"Engery last day", "kWh", CardType::NoCard}},
	{}
};

const ModbusRegisterDescription regS2[3] PROGMEM =
{
	{3022, ModbusRegisterValueType::U16, 0.1, "3022", {"DC voltage", "V", CardType::NoCard}},
	{3023, ModbusRegisterValueType::U16, 0.1, "3023", {"DC current", "A", CardType::NoCard}},
	{}
};

const ModbusRegisterDescription regS3[6] PROGMEM =
{
	{3036, ModbusRegisterValueType::U16, 0.1, "3036", {"AC voltage", "V", CardType::Generic}},
	{3039, ModbusRegisterValueType::U16, 0.1, "3039", {"AC current", "A", CardType::Generic}},
	{3041, ModbusRegisterValueType::U16, 1.0, "3041", {"Working mode", "", CardType::NoCard}},
	{3042, ModbusRegisterValueType::U16, 0.1, "3042", {"Temperature", "C", CardType::Temperature}},
	{3043, ModbusRegisterValueType::U16, 0.01, "3043", {"Frequency", "Hz", CardType::Generic}},
	{}
};

const ModbusRegisterDescription regS4[2] PROGMEM =
{
	{3050, ModbusRegisterValueType::U16, 0.01, "3050", {"Power limit", "%", CardType::Generic}},
	{}
};

const ModbusRegisterDescription regS5[7] PROGMEM =
{
	{3067, ModbusRegisterValueType::U16, 1.0, "3067", {"Fault code 01", "", CardType::NoCard}},
	{3068, ModbusRegisterValueType::U16, 1.0, "3068", {"Fault code 02", "", CardType::NoCard}},
	{3069, ModbusRegisterValueType::U16, 1.0, "3069", {"Fault code 03", "", CardType::NoCard}},
	{3070, ModbusRegisterValueType::U16, 1.0, "3070", {"Fault code 04", "", CardType::NoCard}},
	{3071, ModbusRegisterValueType::U16, 1.0, "3071", {"Fault code 05", "", CardType::NoCard}},
	{3072, ModbusRegisterValueType::U16, 1.0, "3072", {"Working status", "", CardType::NoCard}},
	{}
};

const DeviceRegisterSet inverterRegistersInput[] PROGMEM =
{
	{3009, 8, regS1},
	{3022, 2, regS2},
	{3036, 8, regS3},
	{3050, 1, regS4},
	{3067, 6, regS5},
	// or inverterRegistersPower
	{3088, 3, regF3},
	{}
};

/* ----------------- Solis 4G / 5G Holding Registers ---------------- */

const ModbusRegisterDescription regW1[2] PROGMEM =
{
	{3052, ModbusRegisterValueType::U16, 0.01, "3052", {"Power limit set", "%", CardType::Generic}},
	{}
};

const ModbusRegisterDescription regW2[2] PROGMEM =
{
	{3070, ModbusRegisterValueType::U16, 1.0, "3070", {"Power limit switch", "0=ON, 1=OFF", CardType::StatusInverted}},
	{}
};

const DeviceRegisterSet inverterHoldingRegisters[3] PROGMEM =
{
	{3052, 1, regW1},
	{3070, 1, regW2},
	{}
};

/* ----------------- Solis 4G / 5G Discrete Input Registers (Status) ---------------- */

const ModbusRegisterDescription regD1[11] PROGMEM =
{
	{2501, ModbusRegisterValueType::DI, 1.0, "2501", {"Grid Over Voltage", "", CardType::NoCard}},
	{2502, ModbusRegisterValueType::DI, 1.0, "2502", {"Grid Under Voltage", "", CardType::NoCard}},
	{2503, ModbusRegisterValueType::DI, 1.0, "2503", {"Grid Over Frequency", "", CardType::NoCard}},
	{2504, ModbusRegisterValueType::DI, 1.0, "2504", {"Grid Under Frequency", "", CardType::NoCard}},
	{2505, ModbusRegisterValueType::DI, 1.0, "2505", {"Grid wrong polarity", "", CardType::NoCard}},
	{2506, ModbusRegisterValueType::DI, 1.0, "2506", {"No Grid", "", CardType::NoCard}},
	{2507, ModbusRegisterValueType::DI, 1.0, "2507", {"Grid Unbalance", "", CardType::NoCard}},
	{2508, ModbusRegisterValueType::DI, 1.0, "2508", {"Grid Frequency Fluctuation", "", CardType::NoCard}},
	{2509, ModbusRegisterValueType::DI, 1.0, "2509", {"Grid Over Current", "", CardType::NoCard}},
	{2510, ModbusRegisterValueType::DI, 1.0, "2510", {"Grid Current Tracking Fault", "", CardType::NoCard}},
	{}
};

const ModbusRegisterDescription regD2[9] PROGMEM =
{
	{2566, ModbusRegisterValueType::DI, 1.0, "2566", {"Normal Operation", "", CardType::Status}},
	{2567, ModbusRegisterValueType::DI, 1.0, "2567", {"Initial Standby", "", CardType::Status}},
	{2568, ModbusRegisterValueType::DI, 1.0, "2568", {"Control to shutdown", "", CardType::NoCard}},
	{2569, ModbusRegisterValueType::DI, 1.0, "2569", {"Fault to shutdown", "", CardType::NoCard}},
	{2570, ModbusRegisterValueType::DI, 1.0, "2570", {"Standby", "", CardType::NoCard}},
	{2571, ModbusRegisterValueType::DI, 1.0, "2571", {"Derating", "", CardType::NoCard}},
	{2572, ModbusRegisterValueType::DI, 1.0, "2572", {"Limiting", "", CardType::Status}},
	{2573, ModbusRegisterValueType::DI, 1.0, "2573", {"Backup OV Load", "", CardType::NoCard}},
	{}
};

const DeviceRegisterSet inverterDiscreteInputRegisters[3] PROGMEM =
{
	{2501, 10, regD1},
	{2566, 8, regD2},
	{}
};

/* ----------------- Countis E04 ---------------- */

const ModbusRegisterDescription regC1[4] PROGMEM =
{
	{50520, ModbusRegisterValueType::U32, 0.01, "50520", {"Simple voltage : V1", "V", CardType::Generic}},
	{50526, ModbusRegisterValueType::U32, 0.01, "50526", {"Frequency", "Hz", CardType::Generic}},
	{50528, ModbusRegisterValueType::U32, 0.001, "50528", {"Current : I1", "A", CardType::Generic}},
	{}
};

const ModbusRegisterDescription regC2[5] PROGMEM =
{
	{50536, ModbusRegisterValueType::S32, 10, "50536", {"Sum Active Power", "W", CardType::Generic}},
	{50538, ModbusRegisterValueType::S32, 10, "50538", {"Sum Reactive Power", "var", CardType::Generic}},
	{50540, ModbusRegisterValueType::U32, 0.1, "3023", {"Sum Apparent Power", "VA", CardType::Generic}},
	{50540, ModbusRegisterValueType::S32, 0.001, "3023", {"Sum Power Factor", "", CardType::Generic}},
	{}
};

const ModbusRegisterDescription regC3[6] PROGMEM =
{
	{3036, ModbusRegisterValueType::U16, 0.1, "3036", {"AC voltage", "V", CardType::Generic}},
	{3039, ModbusRegisterValueType::U16, 0.1, "3039", {"AC current", "A", CardType::Generic}},
	{3041, ModbusRegisterValueType::U16, 0.1, "3041", {"Working mode", "", CardType::Generic}},
	{3042, ModbusRegisterValueType::U16, 0.1, "3042", {"Temperature", "C", CardType::Generic}},
	{3043, ModbusRegisterValueType::U16, 0.01, "3043", {"Frequency", "Hz", CardType::Generic}},
	{}
};

const ModbusRegisterDescription regC4[2] PROGMEM =
{
	{3050, ModbusRegisterValueType::U16, 0.001, "3050", {"Power limit", "%", CardType::Generic}}	,
	{}
};

const DeviceRegisterSet countisE04Registers[5] PROGMEM =
{
	{3009, 8, regC1},
	{3022, 2, regC2},
	{3036, 8, regC3},
	{3050, 1, regC4},
	{}
};

// clang-format on

InverterInputValuesPower::InverterInputValuesPower()
{
    deviceRegisters = inverterRegistersPower;
    initRegisters(deviceRegisters);
}

String InverterInputValuesPower::getName() const
{
    //return {F("IVP")};
	return {F("ivp")};
}

InverterInputValues::InverterInputValues()
{
    deviceRegisters = inverterRegistersInput;
    initRegisters(deviceRegisters);
}

String InverterInputValues::getName() const
{
    //return {F("IV")};
    return {F("iv")};
}

InverterHoldingRegisters::InverterHoldingRegisters()
{
    deviceRegisters = inverterHoldingRegisters;
    initRegisters(deviceRegisters);
}

String InverterHoldingRegisters::getName() const
{
    //return {F("HR")};
	return {F("hr")};
}

InverterHoldingRegisters::RegisterType InverterHoldingRegisters::getRegisterType() const
{
    return RegisterType::HoldingRegisters;
}


InverterDiscreteInputs::InverterDiscreteInputs()
{
    deviceRegisters = inverterDiscreteInputRegisters;
    initRegisters(deviceRegisters);
}

String InverterDiscreteInputs::getName() const
{
    //return {F("DI")};
	return {F("di")};
}

InverterDiscreteInputs::RegisterType InverterDiscreteInputs::getRegisterType() const
{
    return RegisterType::DiscreteInputs;
}

uint16_t InverterDiscreteInputs::getAddressOffset() const
{
    return 0;
}

