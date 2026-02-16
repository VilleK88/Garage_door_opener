#ifndef LIMITSWITCH_H
#define LIMITSWITCH_H
#include <iostream>
#pragma once
#include "hardware/gpio.h"
#include "pico/time.h"
//#include "StateMachine.h"

static constexpr uint LIM_PIN_LEFT = 27;  // Left/Closed
static constexpr uint LIM_PIN_RIGHT = 28; // Right/Open
static constexpr int64_t LIM_DB_US = 20000;

class LimitSwitch {
public:
    explicit LimitSwitch(uint new_pin);
    void init() const;
    void detect_hit(bool &hit, const std::string& text) const;
private:
    uint pin;
    [[nodiscard]] bool is_pressed_debounced() const;
    [[nodiscard]] bool is_pressed_raw() const;
};

#endif