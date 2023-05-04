#include "config.h"
#include "modbus.h"
#include "inverter.h"
#include "energymonitor.h"
#include "mqtt.h"
#include "myTicker.h"
#include "time.h"
#include "led.h"
#include "wifi.h"
#include "version.h"
#include <ESPDash.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <Arduino.h>

// source .platformio/penv/bin/activate
// PYTHONPATH=/home/wenk/.platformio/penv/lib/python3.9/site-packages/
//
// pio run
// esptool.py --port /dev/ttyUSB1 write_flash -fm dout 0x00000 .pio/build/d1_mini/firmware.bin
// pio run -t upload ## OTA upload
//

// If you want to have a static ip you have to enable it in the config.h file
#ifdef staticIP
// Set your Static IP address
IPAddress local_IP(192, 168, 111, 230);
// Set your Gateway IP address
IPAddress gateway(192, 168, 111, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 111, 1);
IPAddress secondaryDNS(192, 168, 111, 2); // optional
#endif

#ifdef MODBUS_USE_HARDWARE_SERIAL
Serial serial();
#else
SoftwareSerial serial(RS485_RX, RS485_TX, false); // RX, TX, Invert signal
#endif

#ifdef DEVICE_ENMON
RS485Interface rs485if(MODBUS_SLAVEID_DEVICE1, serial);
EnergyMonitorInputValues enmon(mqtt_device1_topic, rs485if);
EnergyMonitorInputValuesPower enmonPow(mqtt_device1_topic, rs485if);
EnergyMonitorHoldingRegisters enmonHR(mqtt_device1_topic, rs485if);

#ifdef DEVICE_ENMON2
String part2(mqtt_device2_topic);
RS485Interface rs485if2(MODBUS_SLAVEID_DEVICE2, serial);

EnergyMonitorInputValues enmon2(part2, rs485if2);
EnergyMonitorInputValuesPower enmonPow2(part2, rs485if2);
EnergyMonitorHoldingRegisters enmonHR2(part2, rs485if2);

constexpr std::array<ModbusSlaveDevice *, 6> modbusSlaves{&enmon, &enmonPow, &enmonHR, &enmon2, &enmonPow2, &enmonHR2};
// constexpr std::array<ModbusSlaveDevice *, 4> modbusSlaves{&enmon, &enmonPow, &enmon2, &enmonPow2};
constexpr std::array<ModbusSlaveDevice *, 4> modbusSlavesOther{&enmon, &enmonHR, &enmon2, &enmonHR2};
// constexpr std::array<ModbusSlaveDevice *, 2> modbusSlavesOther{&enmon, &enmon2};
constexpr std::array<ModbusSlaveDevice *, 2> modbusSlavesPower{&enmonPow, &enmonPow2};
#else
// constexpr std::array<ModbusSlaveDevice *, 3> modbusSlaves{&enmon, &enmonPow, &enmonHR};
constexpr std::array<ModbusSlaveDevice *, 2> modbusSlaves{&enmon, &enmonPow};
// constexpr std::array<ModbusSlaveDevice *, 2> modbusSlavesOther{&enmon, &enmonHR};
constexpr std::array<ModbusSlaveDevice *, 1> modbusSlavesOther{&enmon};
constexpr std::array<ModbusSlaveDevice *, 1> modbusSlavesPower{&enmonPow};
#endif

#else
RS485Interface rs485if(MODBUS_SLAVEID_INVERTER, serial);
InverterInputValues inverter(rs485if);
InverterInputValuesPower inverterPow(rs485if);
InverterHoldingRegisters inverterHR(rs485if);
InverterDiscreteInputs inverterDI(rs485if);

constexpr std::array<ModbusSlaveDevice *, 4> modbusSlaves{&inverter, &inverterPow, &inverterHR, &inverterDI};
constexpr std::array<ModbusSlaveDevice *, 3> modbusSlavesOther{&inverter, &inverterHR, &inverterDI};
constexpr std::array<ModbusSlaveDevice *, 1> modbusSlavesPower{&inverterPow};
#endif

constexpr uint8_t mqttPayloadPrecision{5};

MQTTClient MQTTClient;
myTicker ticker;
LED led;

#ifdef ESPDASH_ENABLED
// ESP-Dash
extern AsyncWebServer server;
static ESPDash dashboard(&server);

Card inverterStatusCard(&dashboard, STATUS_CARD, "MODBUS slaves");
using Cards = std::map<uint32_t, Card *>;
static Cards cards;
using Statistics = std::map<uint32_t, Statistic *>;
static Statistics statistics;

String getCardName(UIDescription const &card)
{
    String desc(card.name);

    if (card.type != CardType::Status && card.type != CardType::StatusInverted)
    {
        desc += " [";
        desc += card.unit;
        desc += "]";
    }

    return desc;
}

uint8_t getCardType(CardType type)
{
    switch (type)
    {
    case CardType::NoCard:
        return GENERIC_CARD;
    case CardType::Generic:
        return GENERIC_CARD;
    case CardType::Status:
    case CardType::StatusInverted:
        return STATUS_CARD;
    case CardType::Temperature:
        return TEMPERATURE_CARD;
    }

    return GENERIC_CARD;
}

void createCard(uint8_t id, ModbusRegisterDescription const &reg)
{
    if (reg.card.type == CardType::NoCard)
        return;

    auto name{getCardName(reg.card)};
    uint32_t cid = (static_cast<uint32_t>(id) << 16) + reg.address;
    cards.emplace(cid, new Card(&dashboard, getCardType(reg.card.type), name.c_str()));
}

void createCards(uint8_t id, RegisterValues const &values)
{
    ModbusRegisterDescription regDesc;
    std::for_each(std::begin(values), std::end(values),
                  [id, &regDesc](auto const &reg)
                  {
                      memcpy_P(&regDesc, reg.second.desc, sizeof(ModbusRegisterDescription));
                      createCard(id, regDesc);
                  });
}

void createCards()
{
    statistics.emplace(0, new Statistic(&dashboard, "WIFI SSID", WIFI_SSID));
    statistics.emplace(1, new Statistic(&dashboard, "Hostname", HOSTNAME));
    statistics.emplace(2, new Statistic(&dashboard, "IP Address", ""));
    statistics.emplace(3, new Statistic(&dashboard, "Version", VERSION));
    statistics.emplace(4, new Statistic(&dashboard, "Build Date", COMPILED_TS));

    std::for_each(std::begin(modbusSlaves), std::end(modbusSlaves),
                  [](auto const &slave) { createCards(slave->getServerId(), slave->getRegisterValues()); });

    dashboard.sendUpdates();
}

String getCardValue(double value, UIDescription const &card)
{
    String desc(value);
    desc += card.unit;
    return desc;
}

void updateCard(uint8_t id, ModbusRegisterValue const &value, ModbusRegisterDescription const &reg)
{
    uint32_t cid = (static_cast<uint32_t>(id) << 16) + reg.address;
    auto it{cards.find(cid)};
    if (it != std::end(cards))
    {
        switch (reg.card.type)
        {
        case CardType::StatusInverted:
        {
            String symbol(F("success"));
            String msg(F("on"));

            if (!value.valid)
            {
                symbol = F("danger");
                msg = F("---");
            }
            else if (value.value > 0.0)
            {
                symbol = F("warning");
                msg = F("off");
            }

            it->second->update(msg.c_str(), symbol.c_str());
        }
        break;
        case CardType::Status:
        {
            String symbol(F("success"));
            String msg(F("on"));

            if (!value.valid)
            {
                symbol = F("danger");
                msg = F("---");
            }
            else if (value.value == 0.0)
            {
                symbol = F("warning");
                msg = F("off");
            }

            it->second->update(msg.c_str(), symbol.c_str());
        }
        break;
        default:
            if (!value.valid)
            {
                String msg(F("---"));
                it->second->update(msg);
            }
            else
            {
                it->second->update(getCardValue(value.value, reg.card));
            }
            break;
        }
    }
}

void updateCards(uint8_t id, RegisterValues const &values)
{
    ModbusRegisterDescription regDesc;
    std::for_each(std::begin(values), std::end(values),
                  [id, &regDesc](auto const &reg)
                  {
                      memcpy_P(&regDesc, reg.second.desc, sizeof(ModbusRegisterDescription));
                      updateCard(id, reg.second, regDesc);
                  });
}

void updateCards()
{
    statistics[0]->set("WIFI SSID", WIFI_SSID);
    statistics[1]->set("Hostname", HOSTNAME);
    statistics[2]->set("IP Address", WiFi.localIP().toString().c_str());
    statistics[3]->set("Version", VERSION);
    statistics[4]->set("Build Date", COMPILED_TS);

    std::for_each(std::begin(modbusSlaves), std::end(modbusSlaves),
                  [](auto const &device) { updateCards(device->getServerId(), device->getRegisterValues()); });
    dashboard.sendUpdates();
}
#endif

void loopOthers()
{
    wifi_loop();
    MQTTClient.loop();
}

void initModbusInterfaces()
{
    serial.begin(MODBUS_SLAVE_BAUDRATE);
    rs485if.begin();

#ifdef DEVICE_ENMON2
    rs485if2.begin();
#endif
}

String getRepresentation(ModbusRegisterValue const &reg, ModbusRegisterDescription const &regDesc)
{
    String payload;

    switch (regDesc.type)
    {
    case ModbusRegisterValueType::U16:
        if (regDesc.scale == 1.0)
            payload = String{static_cast<uint16_t>(reg.value)};
        else
            payload = String{reg.value, mqttPayloadPrecision};
        break;
    case ModbusRegisterValueType::S16:
        if (regDesc.scale == 1.0)
            payload = String{static_cast<int16_t>(reg.value)};
        else
            payload = String{reg.value, mqttPayloadPrecision};
        break;
    case ModbusRegisterValueType::U32:
        if (regDesc.scale == 1.0)
            payload = String{static_cast<uint32_t>(reg.value)};
        else
            payload = String{reg.value, mqttPayloadPrecision};
        break;
    case ModbusRegisterValueType::S32:
        if (regDesc.scale == 1.0)
            payload = String{static_cast<int32_t>(reg.value)};
        else
            payload = String{reg.value, mqttPayloadPrecision};
        break;
    case ModbusRegisterValueType::DI:
        if (reg.value == 0.0)
            payload = F("0");
        else
            payload = F("1");
        break;
    case ModbusRegisterValueType::Float:
        payload = String{reg.value, mqttPayloadPrecision};
        break;
    case ModbusRegisterValueType::BCD:
        payload = String{static_cast<uint8_t>(reg.value)};
        break;
    }

    return payload;
}

void sendModbusSlaveValues(ModbusSlaveDevice const &device)
{
    led.yellowOn();

    auto values{device.getRegisterValues()};
    ModbusRegisterDescription regDesc;

    String topicbase{device.getMqttTopic() + device.getName() + "/"};

    std::for_each(std::begin(values), std::end(values),
                  [&topicbase, &regDesc](auto const &reg)
                  {
                      memcpy_P(&regDesc, reg.second.desc, sizeof(ModbusRegisterDescription));
                      MQTTClient.sendPayload(topicbase + regDesc.mqttTopic, getRepresentation(reg.second, regDesc));
                      loopOthers();
                  });

    MQTTClient.sendPayload(topicbase + F("online"), String{device.isReachable()});

    // if (device.hasErrorCodeChanged())
    {
        MQTTClient.sendPayload(topicbase + F("error"), String{device.getLastErrorCode()});
        MQTTClient.sendPayload(topicbase + F("tries"), String{device.getRetriesNeeded()});
        MQTTClient.sendPayload(topicbase + F("errcnt"), String{device.getErrorCount()});
    }

    led.yellowOff();
}

void sendModbusSlaveValues()
{
    std::for_each(std::begin(modbusSlaves), std::end(modbusSlaves), [](auto const &device) { sendModbusSlaveValues(*device); });
}

void setup()
{
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);
    led.begin();
    led.yellowOn();

    Serial.begin(115200);

    // Init Serial monitor
    while (!Serial)
    {
    }

    Serial.println(F("setup"));

    delay(10000);

    initModbusInterfaces();

#ifdef DEVICE_ENMON
    MQTTClient.addModbusSlave(enmonHR);
#ifdef DEVICE_ENMON2
    MQTTClient.addModbusSlave(enmonHR2);
#endif
#else
    MQTTClient.addModbusSlave(inverterHR);
#endif

#ifdef ESPDASH_ENABLED
    createCards();
#endif

    wifi_setup();

    ticker.begin();    
}

void showInverterStatus()
{
    auto reachable{std::all_of(std::begin(modbusSlaves), std::end(modbusSlaves), [](auto const &slave) { return slave->isReachable(); })};

    if (reachable)
    {
#ifdef ESPDASH_ENABLED
        inverterStatusCard.update(String(F("online")), "success");
        dashboard.sendUpdates();
#endif
        led.disableNightblink();
        led.greenOn();
    }
    else
    {
        led.greenOff();
        led.enableNightBlink();
#ifdef ESPDASH_ENABLED
        inverterStatusCard.update(String(F("offline")), "danger");
        dashboard.sendUpdates();
#endif
    }
}

void updateInverterValues()
{
    if (ticker.isTimeoutShort())
    {
        // MQTTClient.sendPayload(mqtt_status_tick_topic, "1");
        ticker.resetTimeoutShort();

        std::for_each(std::begin(modbusSlavesPower), std::end(modbusSlavesPower),
                      [](auto const &slave)
                      {
                          slave->request();
                          sendModbusSlaveValues(*slave);
                          loopOthers();
                      });

        loopOthers();
        // MQTTClient.sendPayload(mqtt_status_tick_topic, "0");
    }

    if (ticker.isTimeoutLong())
    {
        // MQTTClient.sendPayload(mqtt_status_tick_topic, "2");
        ticker.resetTimeoutLong();

        std::for_each(std::begin(modbusSlavesOther), std::end(modbusSlavesOther),
                      [](auto const &slave)
                      {
                          // MQTTClient.sendPayload(mqtt_status_tick_topic, slave->getName());
                          slave->request();
                          sendModbusSlaveValues(*slave);
                          loopOthers();
                      });

        showInverterStatus();
#ifdef ESPDASH_ENABLED
        updateCards();
#endif
        loopOthers();
        // MQTTClient.sendPayload(mqtt_status_tick_topic, "0");
    }
}

void updateAll()
{
    if (ticker.isTimeoutStatus())
    {
        led.yellowOn();
        MQTTClient.sendStatus();
        ticker.resetTimeoutStatus();
        led.yellowOff();
    }

    updateInverterValues();
}

void loop()
{
    loopOthers();
    updateAll();
}
