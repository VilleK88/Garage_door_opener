//
// Created by ville on 3/4/2026.
//

#ifndef GARAGE_DOOR_OPENER_IGPIOIRQHANDLER_H
#define GARAGE_DOOR_OPENER_IGPIOIRQHANDLER_H
#pragma once
#include "pico/types.h"

class IGpioIrqHandler {
public:
    virtual ~IGpioIrqHandler() = default;
    virtual void on_gpio_irq(uint gpio, uint32_t event_mas) = 0;
};

#endif //GARAGE_DOOR_OPENER_IGPIOIRQHANDLER_H