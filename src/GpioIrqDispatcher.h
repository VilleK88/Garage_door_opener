//
// Created by ville on 3/4/2026.
//

#ifndef GARAGE_DOOR_OPENER_GPIOIRQDISPATCHER_H
#define GARAGE_DOOR_OPENER_GPIOIRQDISPATCHER_H
#pragma once
#include "pico/types.h"
#include <cstddef>

#include "IGpioIrqHandler.h"

class GpioIrqDispatcher {
public:
    static void register_handler(IGpioIrqHandler* h);
    static void init();
    static void gpio_irq_thunk(uint gpio, uint32_t event_mask);
private:
    static constexpr size_t MAX_HANDLERS = 8;
    static inline IGpioIrqHandler* handlers_[MAX_HANDLERS]{};
    static inline size_t count_{0};
};

#endif //GARAGE_DOOR_OPENER_GPIOIRQDISPATCHER_H