#ifndef LED_H
#define LED_H
#pragma once
#include "utils/events.h"
#include <array>
#include <cstdint>

#include "pico/types.h"
#include "hardware/pwm.h"

enum class LedMode : uint8_t {
    Idle,
    Moving,
    Calib,
    Error
};

class LedController {
public:
    LedController();

    void init_leds() const;

    void light_switch(const event_t &event) const;
    void set_brightness(uint led, uint brightness) const;

    static constexpr uint CLK_DIV = 125; // PWM clock divider
    static constexpr uint TOP = 999; // PWM counter top value

    static constexpr uint8_t LIGHT_ON = 1;
    static constexpr uint8_t LIGHT_OFF = 0;

    static constexpr uint8_t BR_RATE = 4; // Step size for brightness changes
    static constexpr uint16_t MAX_BR = TOP + 1; // Max brightness
    static constexpr uint BR_MID = MAX_BR / 2; // 50% brightness level

    static constexpr uint LED0{22};
    static constexpr uint LED1{21};
    static constexpr uint LED2{20};

    bool blink{false};

private:
    std::array<uint, 3> led_pins{LED0, LED1, LED2};
    uint32_t last_blink_ms{0};
    //bool blink_state{false};

    LedMode current_mode{LedMode::Idle};

    uint clamp(int br);
};

#endif