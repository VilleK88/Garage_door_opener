#ifndef LIMITSWITCH_H
#define LIMITSWITCH_H
#pragma once
#include <iostream>
#include "pico/time.h"

class LimitSwitch {
public:
    explicit LimitSwitch(uint new_pin);
    void init() const;
    [[nodiscard]] bool detect_hit(const std::string& text) const;

    static constexpr uint LIM_PIN_LEFT = 27;  // Left/Closed
    static constexpr uint LIM_PIN_RIGHT = 28; // Right/Open
    static constexpr int64_t LIM_DB_US = 20000;
private:
    uint pin;
    [[nodiscard]] bool is_pressed_debounced() const;
};

#endif