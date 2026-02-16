#ifndef LIMITSWITCH_H
#define LIMITSWITCH_H
#include <iostream>
#pragma once
#include "hardware/gpio.h"
#include "pico/time.h"

static constexpr uint LIM_PIN_LEFT = 27;  // Left/Closed
static constexpr uint LIM_PIN_RIGHT = 28; // Right/Open

class LimitSwitch {
public:
    explicit LimitSwitch(uint new_pin);
    void init() const;
    void detect_hit(bool &hit, const std::string& text) const;
private:
    uint pin;
    [[nodiscard]] bool is_pressed_debounced(uint32_t ms = 20) const;
    [[nodiscard]] bool is_pressed_raw() const;
};

#endif