#ifndef ROTARYENCODER_H
#define ROTARYENCODER_H
#pragma once
#include "pico/util/queue.h"
#include "hardware/gpio.h"
#include <cstdint>

static constexpr uint ENC_A = 14;
static constexpr uint ENC_B = 15;

enum class EncDir : int8_t { CW = +1, CCW = -1 };

struct EncEvent {
    EncDir dir;
};

class RotaryEncoder {
public:
    RotaryEncoder(uint new_pin_a, uint new_pin_b);
    void init();
    bool try_read(EncEvent& out);
    uint a_pin() const { return pin_a; }
    uint b_pin() const { return pin_b; }
    void push_from_isr(const EncEvent& ev) { queue_try_add(&q_, &ev); }
private:
    uint pin_a, pin_b;
    queue_t q_;
};

#endif