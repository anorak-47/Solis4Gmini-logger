#pragma once
#include "main.h"

class Ticker;

class myTicker
{
public:
    myTicker();
    void begin();

    bool isTimeoutStatus();
    void resetTimeoutStatus();

    bool isTimeout10S();
    void resetTimeout10S();

    bool isTimeout30S();
    void resetTimeout30S();

private:
    void attach(uint16_t seconds, std::function<void()> &&func);

    bool timeout10S{};
    bool timeout30S{};
    bool timeoutStatus{};

    std::vector<std::unique_ptr<Ticker>> tickers;
};
