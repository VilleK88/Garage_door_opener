#include "LedController.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

LedController::LedController() {
    init_leds();
}

void LedController::init_leds() const {
    /*for (const auto pin : led_pins) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_put(pin, false);
    }*/
    // Track which PWM slices (0-7) have been initialized
    bool slice_ini[8] = {false};

    // Get default PWM configuration
    pwm_config config = pwm_get_default_config();
    // Set clock divider
    pwm_config_set_clkdiv_int(&config, CLK_DIV);
    // Set wrap (TOP)
    pwm_config_set_wrap(&config, TOP);

    for (auto pin : led_pins) {
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

void LedController::update(const uint32_t now_ms) {
    if (current_mode != LedMode::Error) {
        render_base();
    }
    else {
        render_error(now_ms);
    }
}

void LedController::set_mode(const LedMode mode) {
    current_mode = mode;
    if (current_mode == LedMode::Error) {
        last_blink_ms = to_ms_since_boot(get_absolute_time());
        blink_state = false;
    }
    render_base();
}

void LedController::press_button(const BtnEv ev) const {
    if (current_mode != LedMode::Error) {
        switch (ev) {
            case BtnEv::SW0_EV:
                set_brightness(LED0, BR_MID);
                break;
            case BtnEv::SW1_EV:
                set_brightness(LED1, BR_MID);
                break;
            case BtnEv::SW2_EV:
                set_brightness(LED2, BR_MID);
                break;
            default: break;
        }
    }
}

void LedController::switch_leds(const bool on) const {
    for (const auto pin : led_pins) {
        if (on)
            set_brightness(pin, BR_MID);
        else
            set_brightness(pin, LIGHT_OFF);
    }
}

void LedController::blink_middle_led() {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_blink_ms > 500) {
        last_blink_ms = now;
        blink_state = !blink_state;
        set_brightness(LED2, blink_state ? BR_MID : LIGHT_OFF);
    }
}

void LedController::render_base() const {
    switch_leds(false);
    switch (current_mode) {
        case LedMode::Idle: break;
        case LedMode::Moving: break;
        case LedMode::Calib: break;
        case LedMode::Error: break;
    }
}

void LedController::render_error(const uint32_t now_ms) {
    if (now_ms - last_blink_ms >= 500) {
        last_blink_ms = now_ms;
        blink_state = !blink_state;
    }
    switch_leds(blink_state);
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