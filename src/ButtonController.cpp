//
// Created by ville on 3/3/2026.
//

#include "ButtonController.h"

#include "hardware/gpio.h"

ButtonController::ButtonController() {
    init_buttons();
}

void ButtonController::init_buttons() const {
    const size_t size = pins.size();
    for (int i = 0; i < size; i++) {
        gpio_init(pins[i]); // Initialize GPIO pin
        gpio_set_dir(pins[i], GPIO_IN); // Set as input
        gpio_pull_up(pins[i]); // Enable internal pull-up resistor (button reads high = true when not pressed)
        // Configure button interrupt and callback
        gpio_set_irq_enabled(pins[i], GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
    }
}