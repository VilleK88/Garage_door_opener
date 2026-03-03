#include "LedController.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/time.h"

LedController::LedController() {
    init_leds();
}

void LedController::init_leds() const {
    // Track which PWM slices (0-7) have been initialized
    bool slice_ini[8] = {false};

    // Get default PWM configuration
    pwm_config config = pwm_get_default_config();
    // Set clock divider
    pwm_config_set_clkdiv_int(&config, CLK_DIV);
    // Set wrap (TOP)
    pwm_config_set_wrap(&config, TOP);

    for (const auto pin : led_pins) {
        // Get slice and channel for your GPIO pin
        const uint slice = pwm_gpio_to_slice_num(pin);
        const uint chan = pwm_gpio_to_channel(pin);

        // Disable PWM while configuring
        pwm_set_enabled(slice, false);

        // Initialize each slice once (sets divider and TOP for both A/B)
        if (!slice_ini[slice]) {
            pwm_init(slice, &config, false); // Do not start yet
            slice_ini[slice] = true;
        }

        // Set compare value (CC) to define duty cycle
        pwm_set_chan_level(slice, chan, 0);
        // Select PWM model for your pin
        gpio_set_function(pin, GPIO_FUNC_PWM);
        // Start PWM
        pwm_set_enabled(slice, true);
    }
}

void LedController::light_switch(event_t event) const {
    switch (event.type) {
        case EV_SW0:
            set_brightness(LED2, event.data ? BR_MID : LIGHT_OFF);
            break;
        case EV_SW1:
            set_brightness(LED1, event.data ? BR_MID : LIGHT_OFF);
            break;
        case EV_SW2:
            set_brightness(LED0, event.data ? BR_MID : LIGHT_OFF);
            break;
        default:
            break;
    }
}

/*
 * Update PWM duty cycle of LED.
 * - slice = PWM slice (0–7)
 * - chan  = PWM channel (A/B)
 */
void LedController::set_brightness(const uint led, const uint brightness) const {
    const uint slice = pwm_gpio_to_slice_num(led);
    const uint chan = pwm_gpio_to_channel(led);
    pwm_set_chan_level(slice, chan, brightness);
}

/*
 * Clamp brightness to valid PWM range [0, MAX_BR].
 */
uint LedController::clamp(const int br) {
    // Limit brightness value to valid PWM range [0, MAX_BR]
    if (br < 0) return 0; // Lower bound
    if (br > MAX_BR) return MAX_BR; // Upper bound
    return br; // Within range
}