//
// Created by ville on 2/25/2026.
//

#ifndef GARAGE_DOOR_OPENER_ROTARYENCODER_H
#define GARAGE_DOOR_OPENER_ROTARYENCODER_H
#pragma once
#include "IGpioIrqHandler.h"
#include "pico/types.h"

class RotaryEncoder final : public IGpioIrqHandler {
public:
    RotaryEncoder();
    [[nodiscard]] int get_position() const { return position; }
    void on_gpio_irq(uint gpio, uint32_t event_mask) override;
private:
    void init() const;
    uint ENC_A{14};
    uint ENC_B{15};
    volatile int position{0};
    static RotaryEncoder* instance;
};

#endif //GARAGE_DOOR_OPENER_ROTARYENCODER_H