#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/util/queue.h"
#include "main.h"
#include "src/StateMachine.h"
#include "src/RotaryEncoder.h"

// Global event queue used by ISR (Interrupt Service Routine) and main loop
queue_t events;
//RotaryEncoder* g_enc = nullptr;

int main() {
    // Initialize chosen serial port
    stdio_init_all();
    // Initialize buttons
    init_buttons();
    // Initialize Rotary Encoder
    init_encoder();

    int position = 0;

    StateMachine sm;

    event_t event;
    while (true) {

        while (queue_try_remove(&events, &event)) {
            if (event.type == EV_SW0 && event.data == 1) {
                std::cout << "Left button pressed.\n";
                sm.next_state(CurrentState::start_calib);
            }
            if (event.type == EV_SW1 && event.data == 1) {
                std::cout << "Middle button pressed.\n";
            }
            if (event.type == EV_SW2 && event.data == 1) {
                std::cout << "Right button pressed.\n";
            }
            if (event.type == EVENT_ENCODER) {
                position += event.data;
                sm.update_position(position);
                std::cout << "Position: " << position << "\n";
            }
        }

        sm.run_sm();
    }
}

void gpio_callback(uint const gpio, uint32_t const event_mask) {
    const uint32_t now = to_ms_since_boot(get_absolute_time());

    if (gpio == SW0) {
        static uint32_t last_ms_r = 0; // Store last interrupt time
        // Detect button release (rising edge)
        if (event_mask & GPIO_IRQ_EDGE_RISE && now - last_ms_r >= DEBOUNCE_MS) {
            last_ms_r = now;
            constexpr event_t event = { .type = EV_SW0, .data = 0 };
            queue_try_add(&events, &event); // Add event to queue
        }
        // Detect button press (falling edge)
        if (event_mask & GPIO_IRQ_EDGE_FALL && now - last_ms_r >= DEBOUNCE_MS) {
            last_ms_r = now;
            constexpr event_t event = { .type = EV_SW0, .data = 1 };
            queue_try_add(&events, &event); // Add event to queue
        }
    }

    // Button press/release with debounce to ensure one physical press counts as one event
    if (gpio == SW1) {
        static uint32_t last_ms_m = 0; // Store last interrupt time
        // Detect button release (rising edge)
        if (event_mask & GPIO_IRQ_EDGE_RISE && now - last_ms_m >= DEBOUNCE_MS) {
            last_ms_m = now;
            constexpr event_t event = { .type = EV_SW1, .data = 0 };
            queue_try_add(&events, &event); // Add event to queue
        }
        // Detect button press (falling edge)
        if (event_mask & GPIO_IRQ_EDGE_FALL && now - last_ms_m >= DEBOUNCE_MS){
            last_ms_m = now;
            constexpr event_t event = { .type = EV_SW1, .data = 1 };
            queue_try_add(&events, &event); // Add event to queue
        }
    }

    if (gpio == SW2) {
        static uint32_t last_ms_l = 0; // Store last interrupt time
        // Detect button release (rising edge)
        if (event_mask & GPIO_IRQ_EDGE_RISE && now - last_ms_l >= DEBOUNCE_MS) {
            last_ms_l = now;
            constexpr event_t event = { .type = EV_SW2, .data = 0 };
            queue_try_add(&events, &event); // Add event to queue
        }
        // Detect button press (falling edge)
        if (event_mask & GPIO_IRQ_EDGE_FALL && now - last_ms_l >= DEBOUNCE_MS) {
            last_ms_l = now;
            constexpr event_t event = { .type = EV_SW2, .data = 1 };
            queue_try_add(&events, &event); // Add event to queue
        }
    }

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

//------------------------------------------------------------
// Configure buttons as inputs and attach IRQ callbacks.
//------------------------------------------------------------
void init_buttons() {
    // Initialize event queue for Interrupt Service Routine (ISR)
    queue_init(&events, sizeof(event_t), 128);

    for (int i = 0; i < BUTTONS_SIZE; i++) {
        gpio_init(buttons[i]); // Initialize GPIO pin
        gpio_set_dir(buttons[i], GPIO_IN); // Set as input
        gpio_pull_up(buttons[i]); // Enable internal pull-up resistor (button reads high = true when not pressed)
        // Configure button interrupt and callback
        gpio_set_irq_enabled_with_callback(buttons[i], GPIO_IRQ_EDGE_FALL |
            GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    }
}

void init_encoder() {
    gpio_init(ENC_A);
    gpio_set_dir(ENC_A, GPIO_IN);
    gpio_init(ENC_B);
    gpio_set_dir(ENC_B, GPIO_IN);
    gpio_set_irq_enabled(ENC_A, GPIO_IRQ_EDGE_RISE, true);
}