#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/util/queue.h"
#include "main.h"
#include "src/SwitchLedUnit.h"

// Global event queue used by ISR (Interrupt Service Routine) and main loop
queue_t events;

int main() {
    constexpr uint buttons[] = {SW0, SW1, SW2};
    constexpr uint leds[] = {LED3, LED2, LED1};
    std::vector<std::shared_ptr<SwitchLedUnit>> sw_led_units;

    // Initialize chosen serial port
    stdio_init_all();
    // Initialize SwitchLedUnits
    init_sw_led_units(sw_led_units, buttons, leds);
    // Initialize buttons
    init_buttons(sw_led_units);
    // Initialize LED pins
    init_leds(sw_led_units);

    event_t event;
    while (true) {

        while (queue_try_remove(&events, &event)) {
            const uint32_t now_ms = to_ms_since_boot(get_absolute_time());

            if (event.type == EV_SW0)
                sw_led_units[0]->btn_event(event.data, now_ms, LED_OFF_DELAY_MS);
            if (event.type == EV_SW1)
                sw_led_units[1]->btn_event(event.data, now_ms, LED_OFF_DELAY_MS);
            if (event.type == EV_SW2)
                sw_led_units[2]->btn_event(event.data, now_ms, LED_OFF_DELAY_MS);
        }

        const uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        for (auto& u : sw_led_units) {
            u->update_led(now_ms);
        }
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
}

void init_sw_led_units(std::vector<std::shared_ptr<SwitchLedUnit>>& sw_led_units, const uint* buttons, const uint* leds) {
    sw_led_units.clear();
    sw_led_units.reserve(3);
    for (int i = 0; i < 3; i++) {
        sw_led_units.push_back(std::make_shared<SwitchLedUnit>(buttons[i], leds[i], false, false, LIGHTS_OFF));
    }
}

void init_buttons(const std::vector<std::shared_ptr<SwitchLedUnit>>& sw_led_units) {
    // Initialize event queue for Interrupt Service Routine (ISR)
    // 32 chosen as a safe buffer size: large enough to handle bursts of interrupts
    // without losing events, yet small enough to keep RAM usage minimal.
    queue_init(&events, sizeof(event_t), 32);

    for (int i = 0; i < BUTTONS_SIZE; i++) {
        sw_led_units[i]->init_button();
    }

    gpio_set_irq_enabled_with_callback(SW0,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
    true, &gpio_callback);

    gpio_set_irq_enabled(SW1,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true);

    gpio_set_irq_enabled(SW2,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
        true);
}

void init_leds(const std::vector<std::shared_ptr<SwitchLedUnit>>& sw_led_units) {
    // Track which PWM slices (0-7) have been initialized
    bool slice_ini[8] = {false};

    // Get default PWM configuration
    pwm_config config = pwm_get_default_config();
    // Set clock divider
    pwm_config_set_clkdiv_int(&config, CLK_DIV);
    // Set wrap (TOP)
    pwm_config_set_wrap(&config, TOP);

    for (int i = 0; i < LEDS_SIZE; i++) {
        sw_led_units[i]->init_led(slice_ini, config);
    }
}

uint clamp(const int br) {
    // Limit brightness value to valid PWM range [0, MAX_BR]
    if (br < 0) return 0; // Lower bound
    if (br > MAX_BR) return MAX_BR; // Upper bound
    return static_cast<uint>(br); // Within range
}