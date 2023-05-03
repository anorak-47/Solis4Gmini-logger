#pragma once

class LED
{
public:
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
