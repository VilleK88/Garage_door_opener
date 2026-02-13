#ifndef SWITCHLEDUNIT_H
#define SWITCHLEDUNIT_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"

class SwitchLedUnit {
public:
    SwitchLedUnit(uint new_btn_pin, uint new_led_pin, bool new_btn_stat, bool new_led_stat, uint new_bright);
    void init_button() const;
    void init_led(bool slice_ini[], pwm_config config) const;
    void set_brightness(uint new_bright);
    void btn_event(int32_t btn_state, uint32_t now_ms, uint32_t off_delay_ms);
    void update_led(uint32_t now_ms);
private:
    uint btn_pin;
    uint led_pin;
    bool btn_pressed;
    bool led_on;
    uint bright;

    bool led_pending_off = false;
    uint32_t led_off_ms = 0;
};

#endif