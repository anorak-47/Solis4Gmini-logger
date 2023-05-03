#include "myTicker.h"
#include "config.h"
#include <Arduino.h>
#include <Ticker.h>

Ticker mqttStatusTicker;
Ticker readTicker;

myTicker::myTicker() = default;
myTicker::~myTicker() = default;

bool myTicker::isTimeoutStatus()
{

    return timeoutStatus;
}

void myTicker::resetTimeoutStatus()
{
    timeoutStatus = false;
}

bool myTicker::isTimeoutShort()
{

    return timeoutShort;
}

void myTicker::resetTimeoutShort()
{
	timeoutShort = false;
}

bool myTicker::isTimeoutLong()
{

    return timeoutLong;
}

void myTicker::resetTimeoutLong()
{
	timeoutLong = false;
}

void myTicker::attach(uint16_t seconds, std::function<void()> &&func)
{
    auto ticker{std::make_unique<Ticker>()};
    ticker->attach(seconds, func);
    tickers.emplace_back(std::move(ticker));
}

void myTicker::begin()
{
	attach(TIMEOUT_SHORT, [this](){timeoutShort = true;});
	attach(TIMEOUT_LONG, [this](){timeoutLong = true;});
	attach(mqttStatusIntveral, [this](){timeoutStatus = true;});
}
