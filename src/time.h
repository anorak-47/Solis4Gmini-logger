#pragma once
#include <cinttypes>

class timeControl
{
public:
    timeControl();
    void begin();
    void loop();
    void print();

    void getTime(uint8_t *_hour, uint8_t *_minute);
    void getDate(uint16_t *_year, uint8_t *_month, uint8_t *_day);
};
