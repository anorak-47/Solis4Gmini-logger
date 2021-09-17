#include "myTicker.h"
#include <Ticker.h>

Ticker mqttStatusTicker;
Ticker readTicker;

myTicker::myTicker()
{
}

bool myTicker::isTimeoutStatus()
{

    return timeoutStatus;
}

void myTicker::resetTimeoutStatus()
{
    timeoutStatus = false;
}

bool myTicker::isTimeout10S()
{

    return timeout10S;
}

void myTicker::resetTimeout10S()
{
	timeout10S = false;
}

bool myTicker::isTimeout30S()
{

    return timeout30S;
}

void myTicker::resetTimeout30S()
{
	timeout30S = false;
}

void myTicker::attach(uint16_t seconds, std::function<void()> &&func)
{
    auto ticker{std::make_unique<Ticker>()};
    ticker->attach(seconds, func);
    tickers.emplace_back(std::move(ticker));
}

/*
Starts ticker
*/
void myTicker::begin()
{
	attach(10, [this](){timeout10S = true;});
	attach(30, [this](){timeout30S = true;});
	attach(mqttStatusIntveral, [this](){timeoutStatus = true;});
}
