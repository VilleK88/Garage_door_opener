//
// Created by ville on 2/25/2026.
//

#ifndef GARAGE_DOOR_OPENER_ROTARYENCODER_H
#define GARAGE_DOOR_OPENER_ROTARYENCODER_H
#include "pico/types.h"
#include "pico/util/queue.h"

class RotaryEncoder {
public:
    RotaryEncoder();
    void init_encoder() const;
    [[nodiscard]] int get_position() const { return position; }
    void on_gpio_irq(uint gpio, uint32_t event_mask) const;
private:
    static void gpio_irq_trampoline(uint gpio, uint32_t events);

    uint ENC_A{14};
    uint ENC_B{15};

    volatile int position{0};
    static RotaryEncoder* instance;
};

#endif //GARAGE_DOOR_OPENER_ROTARYENCODER_H