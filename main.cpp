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
#include "src/LimitSwitch.h"

// Global event queue used by ISR (Interrupt Service Routine) and main loop
queue_t events;
RotaryEncoder* g_enc = nullptr;

int main() {
    // Initialize chosen serial port
    stdio_init_all();
    // Initialize buttons
    init_buttons();

    RotaryEncoder enc(ENC_A, ENC_B);
    enc.init();
    g_enc = &enc;

    /*LimitSwitch left_limit(LIM_PIN_LEFT);
    left_limit.init();
    LimitSwitch right_limit(LIM_PIN_RIGHT);
    right_limit.init();*/

    int position = 0;
    //bool calibrated = false;
    //int max_pos = 0;

    StateMachine sm;

    event_t event;
    while (true) {

        while (queue_try_remove(&events, &event)) {
            if (event.type == EV_SW0 && event.data == 1) {
                std::cout << "Left button pressed.\n";
                sm.next_state(CurrentState::run_motor);
            }
            if (event.type == EV_SW1 && event.data == 1) {
                std::cout << "Middle button pressed.\n";
            }
            if (event.type == EV_SW2 && event.data == 1) {
                std::cout << "Right button pressed.\n";
            }
        }

        EncEvent e;
        while (enc.try_read(e)) {
            position += (e.dir == EncDir::CW) ? 1 : -1;
        }

        /*static bool left_hit = false;
        left_limit.detect_hit(left_hit, "Left");
        static bool right_hit = false;
        right_limit.detect_hit(right_hit, "Right");*/

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

    if (g_enc && gpio == g_enc->a_pin()) {
        if (event_mask & GPIO_IRQ_EDGE_RISE) {
            bool b_state = gpio_get(g_enc->b_pin());
            EncEvent ev{ b_state ? EncDir::CCW : EncDir::CW };
            g_enc->push_from_isr(ev);
        }
    }
}

//------------------------------------------------------------
// Configure buttons as inputs and attach IRQ callbacks.
//------------------------------------------------------------
void init_buttons() {
    // Initialize event queue for Interrupt Service Routine (ISR)
    // 32 chosen as a safe buffer size: large enough to handle bursts of interrupts
    // without losing events, yet small enough to keep RAM usage minimal.
    queue_init(&events, sizeof(event_t), 32);

    for (int i = 0; i < BUTTONS_SIZE; i++) {
        gpio_init(buttons[i]); // Initialize GPIO pin
        gpio_set_dir(buttons[i], GPIO_IN); // Set as input
        gpio_pull_up(buttons[i]); // Enable internal pull-up resistor (button reads high = true when not pressed)
        // Configure button interrupt and callback
        gpio_set_irq_enabled_with_callback(buttons[i], GPIO_IRQ_EDGE_FALL |
            GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
    }
}