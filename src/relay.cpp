#include "relay.h"
#include "main.h"

void Relay::begin()
{
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
}

void Relay::toggle()
{
    digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN));
}

void Relay::setOn()
{
    digitalWrite(RELAY_PIN, HIGH);
}

void Relay::setOff()
{
    digitalWrite(RELAY_PIN, LOW);
}
