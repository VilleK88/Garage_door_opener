#include <iostream>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "SwitchLedUnit.h"
#include "../main.h"

SwitchLedUnit::SwitchLedUnit(uint new_btn_pin, uint new_led_pin, bool new_btn_stat, bool new_led_stat, uint new_bright) :
btn_pin(new_btn_pin), led_pin(new_led_pin), btn_pressed(new_btn_stat), led_on(new_led_stat), bright(new_bright) {}

void SwitchLedUnit::init_button() const {
    gpio_init(btn_pin); // Initialize GPIO pin
    gpio_set_dir(btn_pin, GPIO_IN); // Set as input
    gpio_pull_up(btn_pin); // Enable internal pull-up resistor (button reads high = true when not pressed)
    // Configure button interrupt and callback
}

void SwitchLedUnit::init_led(bool slice_ini[], pwm_config config) const {
    // Get slice and channel for your GPIO pin
    const uint slice = pwm_gpio_to_slice_num(led_pin);
    const uint chan = pwm_gpio_to_channel(led_pin);

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
    gpio_set_function(led_pin, GPIO_FUNC_PWM);
    // Start PWM
    pwm_set_enabled(slice, true);
}

void SwitchLedUnit::set_brightness(const uint new_bright) {
    bright = new_bright;
    const uint slice = pwm_gpio_to_slice_num(led_pin);  // Get PWM slice for LED pin
    const uint chan  = pwm_gpio_to_channel(led_pin); // Get PWM channel (A/B)
    pwm_set_chan_level(slice, chan, bright); // Update duty cycle value
}

void SwitchLedUnit::btn_event(int32_t btn_state, uint32_t now_ms, uint32_t off_delay_ms) {
    if (btn_state == 0) {
        if (!led_pending_off && led_on) {
            led_off_ms = now_ms + off_delay_ms;
            led_pending_off = true;
        }
    }
    if (btn_state == 1) {
        set_brightness(bright > 0 ? bright : BR_MID);
        led_on = true;
        led_pending_off = false;
    }
}

void SwitchLedUnit::update_led(uint32_t now_ms) {
    if (led_pending_off) {
        if ((int32_t)(now_ms - led_off_ms) >= 0) {
            set_brightness(LIGHTS_OFF);
            led_on = false;
            led_pending_off = false;
        }
    }
}