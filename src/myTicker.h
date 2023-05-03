#pragma once
#include <cinttypes>
#include <functional>
#include <vector>
#include <memory>

class Ticker;

class myTicker
{
public:
	myTicker();
	~myTicker();
	
    void begin();

    bool isTimeoutStatus();
    void resetTimeoutStatus();

    bool isTimeoutShort();
    void resetTimeoutShort();

    bool isTimeoutLong();
    void resetTimeoutLong();

private:
    void attach(uint16_t seconds, std::function<void()> &&func);

    bool timeoutShort{};
    bool timeoutLong{};
    bool timeoutStatus{};

    std::vector<std::unique_ptr<Ticker>> tickers;
};
