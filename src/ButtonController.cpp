//
// Created by ville on 3/3/2026.
//

#include "ButtonController.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include "utils/events.h"

ButtonController::ButtonController() {
    init();
}

void ButtonController::init() const {
    const size_t size = pins.size();
    for (int i = 0; i < size; i++) {
        gpio_init(pins[i]); // Initialize GPIO pin
        gpio_set_dir(pins[i], GPIO_IN); // Set as input
        gpio_pull_up(pins[i]); // Enable internal pull-up resistor (button reads high = true when not pressed)
        // Configure button interrupt and callback
        gpio_set_irq_enabled(pins[i], GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
    }
}

void ButtonController::on_gpio_irq(const uint gpio, const uint32_t event_mask) {
    const uint32_t now = to_ms_since_boot(get_absolute_time());

    auto debounce_ok = [&](uint32_t& last_ms) {
        if (now - last_ms >= DEBOUNCE_MS) {
            last_ms = now;
            return true;
        }
        return false;
    };

    if (gpio == SW0) {
        if (debounce_ok(last_sw0_ms_)) {
            if (event_mask & GPIO_IRQ_EDGE_FALL) {
                sw0_down_ = true;
                constexpr event_t e{EV_SW0, 1, {}};
                queue_try_add(&events, &e);
            }
            else if (event_mask & GPIO_IRQ_EDGE_RISE) {
                sw0_down_ = false;
                constexpr event_t e{EV_SW0, 0, {}};
                queue_try_add(&events, &e);
            }
        }
    }

    if (gpio == SW1) {
        if (debounce_ok(last_sw1_ms_)) {
            if (event_mask & GPIO_IRQ_EDGE_FALL) {
                constexpr event_t e{EV_SW1, 1, {}};
                queue_try_add(&events, &e);
            }
            else if (event_mask & GPIO_IRQ_EDGE_RISE) {
                constexpr event_t e{EV_SW1, 0, {}};
                queue_try_add(&events, &e);
            }
        }
    }

    if (gpio == SW2) {
        if (debounce_ok(last_sw2_ms_)) {
            if (event_mask & GPIO_IRQ_EDGE_FALL) {
                sw2_down_ = true;
                constexpr event_t e{EV_SW2, 1, {}};
                queue_try_add(&events, &e);
            }
            else if (event_mask & GPIO_IRQ_EDGE_RISE) {
                sw2_down_ = false;
                constexpr event_t e{EV_SW2, 0, {}};
                queue_try_add(&events, &e);
            }
        }
    }

    if (sw0_down_ && sw2_down_) {
        constexpr event_t e{EV_CALIB, 1, {}};
        queue_try_add(&events, &e);
    }
}