//
// Created by ville on 2/25/2026.
//

#include "RotaryEncoder.h"
#include "../main.h"
#include "hardware/gpio.h"
#include "utils/events.h"

RotaryEncoder::RotaryEncoder() {
    init_encoder();
}

void RotaryEncoder::init_encoder() const {
    gpio_init(ENC_A);
    gpio_set_dir(ENC_A, GPIO_IN);
    gpio_init(ENC_B);
    gpio_set_dir(ENC_B, GPIO_IN);
    gpio_set_irq_enabled_with_callback(ENC_A, GPIO_IRQ_EDGE_RISE,
        true, &gpio_callback);
}

void RotaryEncoder::on_gpio_irq(const uint gpio, const uint32_t event_mask) const {
    if (gpio == ENC_A) {
        if (event_mask & GPIO_IRQ_EDGE_RISE) {
            bool b_state = gpio_get(ENC_B);
            event_t ev;
            ev.type = EVENT_ENCODER;
            ev.data = b_state ? -1 : +1;
            queue_try_add(&events, &ev);
        }
    }
}