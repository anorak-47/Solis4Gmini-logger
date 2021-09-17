#pragma once
#include "main.h"

class LED
{
public:
    LED();
    void begin();

    void blueToggle();
    void greenOn();
    void greenOff();

    void yellowToggle();
    void yellowOn();
    void yellowOff();

    void enableNightBlink();
    void disableNightblink();
    
};
