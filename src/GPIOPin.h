#ifndef GPIOPIN_H
#define GPIOPIN_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"

class GPIOPin {
public:
    GPIOPin(int pin, bool input = true, bool pullup = true, bool invert = false);
    GPIOPin(const GPIOPin &) = delete; // makes the object non-copyable
    ~GPIOPin();
    bool read(); // read state of the GPIO pin
    void write(bool value);
    explicit operator bool(); // returns true if pin is usable and false is dormant
private:
    static uint32_t pins_in_use;
};

#endif