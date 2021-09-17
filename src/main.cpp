#include <Arduino.h>
#include "main.h"

// pio run
// esptool.py --port /dev/ttyUSB1 write_flash -fm dout 0x00000 .pio/build/d1_mini/firmware.bin
// pio run -t upload

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

RS485Interface rs485if;
InverterInputValues inverter;
InverterInputValuesPower inverterPow;
InverterHoldingRegisters inverterHR;
InverterDiscreteInputs inverterDI;

constexpr std::array<ModbusSlaveDevice *, 4> modbusSlaves{&inverter, &inverterPow, &inverterHR, &inverterDI};
constexpr std::array<ModbusSlaveDevice *, 3> modbusSlavesOther{&inverter, &inverterHR, &inverterDI};

myMqtt MQTTClient;
myTicker ticker;
timeControl Clock;
LED led;

// ESP-Dash
AsyncWebServer server(80);
ESPDash dashboard(&server);

uint16_t Year = 2000;
uint8_t Month = 0;
uint8_t Day = 0;
uint8_t Hour = 0;
uint8_t Minute = 0;
bool restart = false;

Card inverterStatusCard(&dashboard, STATUS_CARD, "MODBUS slaves");
using Cards = std::map<uint16_t, Card *>;
Cards cards;

void notFound(AsyncWebServerRequest *request);
String restartAPI();

String getCardName(CardDescription const &card)
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

void createCard(ModbusRegisterDescription const &reg)
{
    if (reg.card.type == CardType::NoCard)
        return;

    auto name{getCardName(reg.card)};
    cards.emplace(reg.address, new Card(&dashboard, getCardType(reg.card.type), name.c_str()));
}

void createCards(RegisterValues const &values)
{
    ModbusRegisterDescription regDesc;
    std::for_each(std::begin(values), std::end(values),
                  [&regDesc](auto const &reg)
                  {
                      memcpy_P(&regDesc, reg.second.reg, sizeof(ModbusRegisterDescription));
                      createCard(regDesc);
                  });
}

void createCards()
{
    std::for_each(std::begin(modbusSlaves), std::end(modbusSlaves), [](auto const &inverter) { createCards(inverter->getRegisterValues()); });
    dashboard.sendUpdates();
}

String getCardValue(double value, CardDescription const &card)
{
    String desc(value);
    desc += card.unit;
    return desc;
}

void updateCard(ModbusRegisterValue const &value, ModbusRegisterDescription const &reg)
{
    auto it{cards.find(reg.address)};
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

void updateCards(RegisterValues const &values)
{
    ModbusRegisterDescription regDesc;
    std::for_each(std::begin(values), std::end(values),
                  [&regDesc](auto const &reg)
                  {
                      memcpy_P(&regDesc, reg.second.reg, sizeof(ModbusRegisterDescription));
                      updateCard(reg.second, regDesc);
                  });
}

void updateCards()
{
    std::for_each(std::begin(modbusSlaves), std::end(modbusSlaves), [](auto const &device) { updateCards(device->getRegisterValues()); });
    dashboard.sendUpdates();
}

void loopOthers()
{
    ArduinoOTA.handle();
    MQTTClient.loop();
    Clock.loop();

    if (restart)
    {
        Serial.println(F("Restarting..."));
        ESP.reset();
    }
}

void preLoop()
{
    for (uint32_t i = 0; i < 10 * 100 * 10; ++i)
    {
        delay(10);
        loopOthers();
    }
}

void initModbusSalves()
{
    rs485if.begin(slaveID_inverter);
    std::for_each(std::begin(modbusSlaves), std::end(modbusSlaves), [](auto const &device) { device->begin(rs485if); });
}

void sendModbusSlaveValuesJSON(ModbusSlaveDevice const &device)
{
    auto values{device.getRegisterValues()};
    ModbusRegisterDescription regDesc;

    String str;

    str += F("{\"");
    str += device.getName();
    str += F("\":{");

    std::for_each(std::begin(values), std::end(values),
                  [&str, &regDesc](auto const &reg)
                  {
                      memcpy_P(&regDesc, reg.second.reg, sizeof(ModbusRegisterDescription));

                      str += String(F("\"")) + regDesc.mqttTopic + String(F("\":"));
                      str += String(reg.second.value);
                      str += ",";
                  });

    str += F(R"("online":)");
    str += String(device.isReachable());
    str += F(R"(}})");

    MQTTClient.sendPayload(device.getMqttTopic(), str);
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
    		payload = String{reg.value};
        break;
    case ModbusRegisterValueType::S16:
    	if (regDesc.scale == 1.0)
    		payload = String{static_cast<int16_t>(reg.value)};
    	else
    		payload = String{reg.value};
        break;
    case ModbusRegisterValueType::U32:
    	if (regDesc.scale == 1.0)
    		payload = String{static_cast<uint32_t>(reg.value)};
    	else
    		payload = String{reg.value};
        break;
    case ModbusRegisterValueType::S32:
    	if (regDesc.scale == 1.0)
    		payload = String{static_cast<int32_t>(reg.value)};
    	else
    		payload = String{reg.value};
        break;
    case ModbusRegisterValueType::DI:
        if (reg.value == 0.0)
            payload = F("0");
        else
            payload = F("1");
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
    MQTTClient.sendPayload(topicbase + F("error"), String{device.getLastErrorCode()});
    MQTTClient.sendPayload(topicbase + F("tries"), String{device.getRetriesNeeded()});
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

    initModbusSalves();
    createCards();

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
    MQTTClient.setModbusSlave(inverterHR);

    /* Start AsyncWebServer */
    server.onNotFound(notFound);
    server.on("/api/restart", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", restartAPI()); });

    server.begin();
    // Start OTA
    ArduinoOTA.begin();
    ArduinoOTA.handle();

    Clock.begin();
    ArduinoOTA.handle();

    Clock.getDate(&Year, &Month, &Day);
    Clock.getTime(&Hour, &Minute);

    ticker.begin();

    // preLoop();

    led.yellowOn();
}

void showInverterStatus()
{
    auto reachable{std::all_of(std::begin(modbusSlaves), std::end(modbusSlaves), [](auto const &slave) { return slave->isReachable(); })};

    if (reachable)
    {
        inverterStatusCard.update(String(F("online")), "success");
        dashboard.sendUpdates();
        led.disableNightblink();
        led.greenOn();
    }
    else
    {
        led.greenOff();
        led.enableNightBlink();
        inverterStatusCard.update(String(F("offline")), "danger");
        dashboard.sendUpdates();
    }
}

void updateInverterValues()
{
    if (ticker.isTimeout10S())
    {
        ticker.resetTimeout10S();
        inverterPow.request();
        sendModbusSlaveValues(inverterPow);

        loopOthers();
    }

    if (ticker.isTimeout30S())
    {
        ticker.resetTimeout30S();

        std::for_each(std::begin(modbusSlavesOther), std::end(modbusSlavesOther),
                      [](auto const &slave)
                      {
                          slave->request();
                          sendModbusSlaveValues(*slave);
                          loopOthers();
                      });

        showInverterStatus();
        updateCards();
        loopOthers();
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
