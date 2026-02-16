#include "RotaryEncoder.h"

RotaryEncoder::RotaryEncoder(uint new_pin_a, uint new_pin_b)
    : pin_a(new_pin_a), pin_b(new_pin_b) {}

void RotaryEncoder::init() {
    gpio_init(pin_a);
    gpio_set_dir(pin_a, GPIO_IN);

    gpio_init(pin_b);
    gpio_set_dir(pin_b, GPIO_IN);

    queue_init(&q_, sizeof(EncEvent), 64);

    gpio_set_irq_enabled(pin_a, GPIO_IRQ_EDGE_RISE, true);
}

bool RotaryEncoder::try_read(EncEvent& out) {
    return queue_try_remove(&q_, &out);
}