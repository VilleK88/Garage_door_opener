//
// Created by ville on 3/4/2026.
//

#include "GpioIrqDispatcher.h"

void GpioIrqDispatcher::register_handler(IGpioIrqHandler* h) {
    if (h) {
        if (count_ < MAX_HANDLERS)
            handlers_[count_++] = h;
    }
}

void GpioIrqDispatcher::init() {}

void GpioIrqDispatcher::gpio_irq_thunk(const uint gpio, const uint32_t event_mask) {
    for (size_t i = 0; i < count_; ++i) {
        handlers_[i]->on_gpio_irq(gpio, event_mask);
    }
}