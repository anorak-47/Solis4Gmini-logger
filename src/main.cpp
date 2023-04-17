#include <Arduino.h>
#include "main.h"

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
SoftwareSerial serial(rs485_RX, rs485_TX, false); // RX, TX, Invert signal
#endif

#ifdef DEVICE_ENMON
RS485Interface rs485if(slaveID_device1, serial);
EnergyMonitorInputValues enmon(mqtt_device1_topic, rs485if);
EnergyMonitorInputValuesPower enmonPow(mqtt_device1_topic, rs485if);
EnergyMonitorHoldingRegisters enmonHR(mqtt_device1_topic, rs485if);

#ifdef DEVICE_ENMON2
String part2(mqtt_device2_topic);
RS485Interface rs485if2(slaveID_device2, serial);

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
RS485Interface rs485if(slaveID_inverter, serial);
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

#if 0
timeControl Clock;
uint16_t Year = 2000;
uint8_t Month = 0;
uint8_t Day = 0;
uint8_t Hour = 0;
uint8_t Minute = 0;
#endif

bool restart = false;

AsyncWebServer server(80);
void notFound(AsyncWebServerRequest *request);
String restartAPI();

#ifdef ESPDASH_ENABLED
// ESP-Dash
ESPDash dashboard(&server);

Card inverterStatusCard(&dashboard, STATUS_CARD, "MODBUS slaves");
using Cards = std::map<uint32_t, Card *>;
Cards cards;

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
                      memcpy_P(&regDesc, reg.second.reg, sizeof(ModbusRegisterDescription));
                      createCard(id, regDesc);
                  });
}

void createCards()
{
    std::for_each(std::begin(modbusSlaves), std::end(modbusSlaves),
                  [](auto const &slave) { createCards(slave->getSlaveId(), slave->getRegisterValues()); });
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
                      memcpy_P(&regDesc, reg.second.reg, sizeof(ModbusRegisterDescription));
                      updateCard(id, reg.second, regDesc);
                  });
}

void updateCards()
{
    std::for_each(std::begin(modbusSlaves), std::end(modbusSlaves),
                  [](auto const &device) { updateCards(device->getSlaveId(), device->getRegisterValues()); });
    dashboard.sendUpdates();
}
#endif

void loopOthers()
{
    ArduinoOTA.handle();
    MQTTClient.loop();
    // Clock.loop();

    if (restart)
    {
        Serial.println(F("Restarting..."));
        ESP.reset();
    }
}

void initModbusInterfaces()
{
    serial.begin(slave_baudrate);
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
    led.yellowOff();

    auto values{device.getRegisterValues()};
    ModbusRegisterDescription regDesc;

    String topicbase{device.getMqttTopic() + device.getName() + "/"};

    std::for_each(std::begin(values), std::end(values),
                  [&topicbase, &regDesc](auto const &reg)
                  {
                      memcpy_P(&regDesc, reg.second.reg, sizeof(ModbusRegisterDescription));
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

    led.yellowOn();
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

    Serial.begin(115200);
    Serial.print(F("setup"));

    initModbusInterfaces();

#ifdef ESPDASH_ENABLED
    createCards();
#endif

    Serial.print(F("Connecting to: "));
    Serial.println(SSID);

#ifdef staticIP
    WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
#endif

    WiFi.begin(SSID, PASSWORD);
    WiFi.hostname(HOSTNAME);

    uint8_t wifiCnt;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        led.yellowToggle();
        Serial.print(".");
        wifiCnt++;
        if (wifiCnt == 254)
        {
            led.yellowOff();
            delay(120000); // 120s
            ESP.restart(); // Restart ESP
        }
    }

    Serial.println(" ");
    Serial.println("Connected");

    // Start mqtt
    MQTTClient.begin();

#ifdef DEVICE_ENMON
    MQTTClient.addModbusSlave(enmonHR);
#ifdef DEVICE_ENMON2
    MQTTClient.addModbusSlave(enmonHR2);
#endif
#else
    MQTTClient.addModbusSlave(inverterHR);
#endif

    MQTTClient.subscribe();

    /* Start AsyncWebServer */
    server.onNotFound(notFound);
    server.on("/api/restart", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", restartAPI()); });

    server.begin();
    // Start OTA
    ArduinoOTA.begin();
    ArduinoOTA.handle();

    // Clock.begin();
    ArduinoOTA.handle();

    // Clock.getDate(&Year, &Month, &Day);
    // Clock.getTime(&Hour, &Minute);

    ticker.begin();
    led.yellowOn();
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
        MQTTClient.sendPayload(mqtt_status_tick_topic, "1");
        ticker.resetTimeoutShort();

        std::for_each(std::begin(modbusSlavesPower), std::end(modbusSlavesPower),
                      [](auto const &slave)
                      {
                          slave->request();
                          sendModbusSlaveValues(*slave);
                          loopOthers();
                      });

        loopOthers();
        MQTTClient.sendPayload(mqtt_status_tick_topic, "0");
    }

    if (ticker.isTimeoutLong())
    {
        MQTTClient.sendPayload(mqtt_status_tick_topic, "2");
        ticker.resetTimeoutLong();

        std::for_each(std::begin(modbusSlavesOther), std::end(modbusSlavesOther),
                      [](auto const &slave)
                      {
                          MQTTClient.sendPayload(mqtt_status_tick_topic, slave->getName());
                          slave->request();
                          sendModbusSlaveValues(*slave);
                          loopOthers();
                      });

        showInverterStatus();
#ifdef ESPDASH_ENABLED
        updateCards();
#endif
        loopOthers();
        MQTTClient.sendPayload(mqtt_status_tick_topic, "0");
    }
}

void updateAll()
{
    if (ticker.isTimeoutStatus())
    {
        led.yellowOff();
        MQTTClient.sendStatus();
        ticker.resetTimeoutStatus();
        led.yellowOn();
    }

    updateInverterValues();
}

void loop()
{
    loopOthers();
    updateAll();
}

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, F("text/plain"), F("Not found"));
}

String restartAPI()
{
    restart = true;
    return F("Restarting");
}
