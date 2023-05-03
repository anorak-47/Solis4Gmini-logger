#include "energymonitor.h"
#include "config.h"

// clang-format off

#ifdef ENERGY_METER_COUNTISE04
/* ----------------- Countis E04 Power Input Registers ---------------- */

const ModbusRegisterDescription regP1[] PROGMEM =
{
	{50536, ModbusRegisterValueType::S32, 10, "50536", {"Sum Active Power", "W", CardType::Generic}},
	{50538, ModbusRegisterValueType::S32, 10, "50538", {"Sum Reactive Power", "var", CardType::Generic}},
	{50540, ModbusRegisterValueType::U32, 0.1, "50540", {"Sum Apparent Power", "VA", CardType::Generic}},
	{50540, ModbusRegisterValueType::S32, 0.001, "50540", {"Sum Power Factor", "", CardType::Generic}},
	{}
};

const ModbusRegisterDescription regP2[] PROGMEM =
{
	{50544, ModbusRegisterValueType::S32, 10, "50544", {"Active Power P1", "W", CardType::Generic}},
	{50550, ModbusRegisterValueType::S32, 10, "50550", {"Reactive Power P1", "var", CardType::Generic}},
	{50556, ModbusRegisterValueType::U32, 0.1, "50556", {"Apparent Power P1", "VA", CardType::Generic}},
	{50562, ModbusRegisterValueType::S32, 0.001, "50562", {"Power Factor P1", "", CardType::Generic}},
	{}
};

const DeviceRegisterSet energyMonitorRegistersPower[] PROGMEM =
{
	{3005, 8, regP1},
	{3056,20, regP2},
	{}
};

/* ----------------- Countis E04 Other Input Registers ---------------- */

const ModbusRegisterDescription regS1[] PROGMEM =
{
	{50520, ModbusRegisterValueType::U32, 0.01, "50520", {"Simple voltage: V1", "V", CardType::Generic}},
	{50526, ModbusRegisterValueType::U32, 0.01, "50526", {"Frequency", "Hz", CardType::Generic}},
	{50528, ModbusRegisterValueType::U32, 0.001, "50528", {"Current: I1", "A", CardType::Generic}},
	{}
};

// Energies
const ModbusRegisterDescription regS2[] PROGMEM =
{
	{50770, ModbusRegisterValueType::U32, 0.001, "50770", {"Total Positive Active Energy Ea+", "Wh", CardType::Generic}},
	{50772, ModbusRegisterValueType::U32, 0.001, "50772", {"Total Positive Reactive Energy Er+", "varh", CardType::Generic}},
	{50774, ModbusRegisterValueType::U32, 0.001, "50774", {"Total Apparent Energy Eap", "VAh", CardType::Generic}},
	{50776, ModbusRegisterValueType::U32, 0.001, "50776", {"Total Negative Active Energy Ea-", "Wh", CardType::Generic}},
	{50778, ModbusRegisterValueType::U32, 0.001, "50778", {"Total Negative Reactive Energy Er-", "varh", CardType::Generic}},
	{50780, ModbusRegisterValueType::U32, 0.001, "50780", {"Partial Positive Active Energy Ea+", "Wh", CardType::Generic}},
	{50782, ModbusRegisterValueType::U32, 0.001, "50782", {"Partial Positive Reactive Energy Er+", "varh", CardType::Generic}},
	{50784, ModbusRegisterValueType::U32, 0.001, "50784", {"Partial Apparent Energy Eap", "W", CardType::Generic}},
	{50786, ModbusRegisterValueType::U32, 0.001, "50786", {"Partial Negative Active Energy Ea-", "Wh", CardType::Generic}},
	{50788, ModbusRegisterValueType::U32, 0.001, "50788", {"Partial Negative Active Energy Er-", "varh", CardType::Generic}},
	{}
};

// Energies in Unit/100
const ModbusRegisterDescription regS3[] PROGMEM =
{
	{50946, ModbusRegisterValueType::U32, 0.1, "50946", {"Total Positive Active Energy Ea+", "Wh", CardType::Generic}},
	{50948, ModbusRegisterValueType::U32, 0.1, "50948", {"Total Positive Reactive Energy Er+", "varh", CardType::Generic}},
	{50952, ModbusRegisterValueType::U32, 0.1, "50952", {"Total Negative Active Energy Ea-", "Wh", CardType::Generic}},
	{50954, ModbusRegisterValueType::U32, 0.1, "50954", {"Total Negative Reactive Energy Er-", "varh", CardType::Generic}},
	{50956, ModbusRegisterValueType::U32, 0.1, "50956", {"Partial Positive Active Energy Ea+", "Wh", CardType::Generic}},
	{50958, ModbusRegisterValueType::U32, 0.1, "50958", {"Partial Positive Reactive Energy Er+", "varh", CardType::Generic}},
	{50962, ModbusRegisterValueType::U32, 0.1, "50962", {"Partial Negative Active Energy Ea-", "Wh", CardType::Generic}},
	{50964, ModbusRegisterValueType::U32, 0.1, "50964", {"Partial Negative Active Energy Er-", "varh", CardType::Generic}},
	{}
};


// Energy measurement - Total
const ModbusRegisterDescription regS4[] PROGMEM =
{
	{25987, ModbusRegisterValueType::U32, 0.001, "50946", {"Total Positive active Energy: Ea+", "Wh", CardType::Generic}},
	{25989, ModbusRegisterValueType::U16, 10.0, "50946", {"Total Residual positive active Energy: rEa+", "Wh", CardType::Generic}},
	{25990, ModbusRegisterValueType::U32, 0.001, "50946", {"Total Negative active Energy: Ea-", "Wh", CardType::Generic}},
	{25992, ModbusRegisterValueType::U16, 10.0, "50946", {"Total Residual negative active Energy: rEa-", "Wh", CardType::Generic}},
	{25993, ModbusRegisterValueType::U32, 0.001, "50946", {"Total Positive reactive Energy: Er+", "varh", CardType::Generic}},
	{25995, ModbusRegisterValueType::U16, 10.0, "50946", {"Total Residual positive reactive Energy: rEr+", "varh", CardType::Generic}},
	{25996, ModbusRegisterValueType::U32, 0.001, "50946", {"Total Negative reactive Energy: Er-", "varh", CardType::Generic}},
	{25998, ModbusRegisterValueType::U16, 10.0, "50946", {"Total Residual negative reactive Energy: rEr-", "varh", CardType::Generic}},
	{25999, ModbusRegisterValueType::U32, 0.001, "50946", {"Total Apparent Energy: Eap", "VAh", CardType::Generic}},
	{26001, ModbusRegisterValueType::U16, 10.0, "50946", {"Total Residual apparent Energy: rEap", "VAh", CardType::Generic}},
	{26002, ModbusRegisterValueType::U32, 0.001, "50946", {"Total positive lagging reactive Energy: Er+ (lagging)", "varh", CardType::Generic}},
	{26004, ModbusRegisterValueType::U16, 10.0, "50946", {"Total residual positive lagging reactive Energy: rEr+ (lagging)", "varh", CardType::Generic}},
	{26005, ModbusRegisterValueType::U32, 0.001, "50946", {"Total negative lagging reactive Energy: Er- (lagging)", "varh", CardType::Generic}},
	{26007, ModbusRegisterValueType::U16, 10.0, "50946", {"Total residual negative lagging reactive Energy: rEr- (lagging)", "varh", CardType::Generic}},
	{26008, ModbusRegisterValueType::U32, 0.001, "50946", {"Total positive leading reactive Energy: Er+ (leading)", "varh", CardType::Generic}},
	{26010, ModbusRegisterValueType::U16, 10.0, "50946", {"Total residual positive leading reactive Energy: rEr+ (leading)", "varh", CardType::Generic}},
	{26011, ModbusRegisterValueType::U32, 0.001, "50946", {"Total negative leading reactive Energy: Er- (leading)", "varh", CardType::Generic}},
	{26013, ModbusRegisterValueType::U16, 10.0, "50946", {"Total residual negative leading reactive Energy: rEr- (leading)", "varh", CardType::Generic}},
	{}
};

const DeviceRegisterSet energyMonitorRegistersInput[] PROGMEM =
{
	{50520, 10, regS1},
	{50770, 20, regS2},
	{50946, 20, regS3},
	{50946, 27, regS4},
	{}
};

/* ----------------- Countis E04 Holding Registers ---------------- */

const ModbusRegisterDescription regW1[] PROGMEM =
{
	// write 0x20 to reset partial energy counters
	{40512, ModbusRegisterValueType::U16, 1.0, "40512", {"Reset Partial Energies", "%", CardType::NoCard}},
	{}
};

const ModbusRegisterDescription regW2[] PROGMEM =
{
	// Reset partial energy counter
	// 0x0001 : Partial Ea+
	// 0x0002 : Partial Er+
	// 0x0004 : Partial Eap
	// 0x0008 : Partial Ea-
	// 0x0010 : Partial Er-
	{57890, ModbusRegisterValueType::U16, 1.0, "57890", {"Reset Partial Energy", "%", CardType::NoCard}},
	{}
};

const DeviceRegisterSet energyMonitorHoldingRegisters[] PROGMEM =
{
	{40512, 1, regW1},
	{57890, 1, regW2},
	{}
};
#endif

/* ----------------- SDM120 Power Input Registers ---------------- */

const ModbusRegisterDescription regP1[] PROGMEM =
{
	{13, ModbusRegisterValueType::Float, 1.0, "30013", {"Active Power", "W", CardType::Generic}},
	{19, ModbusRegisterValueType::Float, 1.0, "30014", {"Apparent Power", "VA", CardType::Generic}},
	{25, ModbusRegisterValueType::Float, 1.0, "30015", {"Reactive Power", "VAr", CardType::Generic}},
	{}
};

const DeviceRegisterSet energyMonitorRegistersPower[] PROGMEM =
{
	{13, 14, regP1},
	{}
};

/* ----------------- SDM120 Other Input Registers ---------------- */

const ModbusRegisterDescription regS1[] PROGMEM =
{
	{1, ModbusRegisterValueType::Float, 1.0, "30001", {"Voltage", "V", CardType::Generic}},
	{7, ModbusRegisterValueType::Float, 1.0, "30007", {"Current", "A", CardType::Generic}},
	{}
};

const ModbusRegisterDescription regS2[] PROGMEM =
{
	{31, ModbusRegisterValueType::Float, 1.0, "30031", {"Power Factor", "", CardType::Generic}},
	{}
};

const ModbusRegisterDescription regS3[] PROGMEM =
{
	{71, ModbusRegisterValueType::Float, 1.0, "30071", {"Frequency", "Hz", CardType::Generic}},
	{73, ModbusRegisterValueType::Float, 1.0, "30073", {"Import Active Energy", "kWh", CardType::Generic}},
	{75, ModbusRegisterValueType::Float, 1.0, "30075", {"Export Active Energy", "kWh", CardType::Generic}},
	{77, ModbusRegisterValueType::Float, 1.0, "30077", {"Import Reactive Energy", "kVArh", CardType::Generic}},
	{79, ModbusRegisterValueType::Float, 1.0, "30079", {"Export Reactive Energy", "kVArh", CardType::Generic}},
	{}
};

const ModbusRegisterDescription regS4[] PROGMEM =
{
	{343, ModbusRegisterValueType::Float, 1.0, "30343", {"Total Active Energy", "kWh", CardType::Generic}},
	{345, ModbusRegisterValueType::Float, 1.0, "30345", {"Total Reactive Energy", "kVArh", CardType::Generic}},
	{}
};


const DeviceRegisterSet energyMonitorRegistersInput[] PROGMEM =
{
	{1, 8, regS1},
	{31, 2, regS2},
	{71, 10, regS3},
	{343, 4, regS4},
	{}
};

/* ----------------- SDM120 Holding Registers ---------------- */

const ModbusRegisterDescription regW1[] PROGMEM =
{
	// write 0x20 to reset partial energy counters
	{63745, ModbusRegisterValueType::BCD, 1.0, "63745", {"Display Scroll", "s", CardType::NoCard}}, // 0-30s, 0=scroll mode off
	{}
};

const DeviceRegisterSet energyMonitorHoldingRegisters[] PROGMEM =
{
	{63745, 1, regW1},
	{}
};

// clang-format on

EnergyMonitorModbusSlaveDevice::EnergyMonitorModbusSlaveDevice(String const& part, RS485Interface &iface) : ModbusSlaveDevice(iface), mqttPathPart(part)
{
}

uint16_t EnergyMonitorModbusSlaveDevice::getAddressOffset() const
{
    return 1;
}

String EnergyMonitorModbusSlaveDevice::getMqttTopic() const
{
	return String(F(mqtt_base_topic)) + mqttPathPart;
}



EnergyMonitorInputValuesPower::EnergyMonitorInputValuesPower(String const& part, RS485Interface &iface) : EnergyMonitorModbusSlaveDevice(part, iface)
{
    deviceRegisters = energyMonitorRegistersPower;
    initRegisters(deviceRegisters);
}

String EnergyMonitorInputValuesPower::getName() const
{
	return {F("pw")};
}

EnergyMonitorInputValues::EnergyMonitorInputValues(String const& part, RS485Interface &iface) : EnergyMonitorModbusSlaveDevice(part, iface)
{
    deviceRegisters = energyMonitorRegistersInput;
    initRegisters(deviceRegisters);
}

String EnergyMonitorInputValues::getName() const
{
    return {F("em")};
}

EnergyMonitorHoldingRegisters::EnergyMonitorHoldingRegisters(String const& part, RS485Interface &iface) : EnergyMonitorModbusSlaveDevice(part, iface)
{
    deviceRegisters = energyMonitorHoldingRegisters;
    initRegisters(deviceRegisters);
}

String EnergyMonitorHoldingRegisters::getName() const
{
	return {F("hr")};
}

EnergyMonitorHoldingRegisters::RegisterType EnergyMonitorHoldingRegisters::getRegisterType() const
{
    return RegisterType::HoldingRegisters;
}
