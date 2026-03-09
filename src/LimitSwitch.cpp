#include "LimitSwitch.h"
#include "hardware/gpio.h"

LimitSwitch::LimitSwitch(const uint new_pin) : pin(new_pin) {}

void LimitSwitch::init() const {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}

bool LimitSwitch::detect_hit(const std::string& text) const {
    if (is_pressed_debounced()) {
        std::cout << text << " hit registered\n";
        return true;
    }
    return false;
}

bool LimitSwitch::is_pressed_debounced() const {
    const absolute_time_t time_0 = get_absolute_time();
    while (absolute_time_diff_us(time_0, get_absolute_time()) < LIM_DB_US) {
        if (gpio_get(pin) != 0)
            return false;
    }
    return true;
}