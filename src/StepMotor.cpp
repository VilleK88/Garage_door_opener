#include "StepMotor.h"
#include "hardware/gpio.h"

StepMotor::StepMotor(const std::array<uint, 4>& pins)
    : coil_pins(pins) {}

void StepMotor::init_coil_pins() const {
    for (auto& coil : coil_pins) {
        gpio_init(coil);
        gpio_set_dir(coil, GPIO_OUT);
        gpio_put(coil, 0);
    }
}

void StepMotor::step(int direction) const {
    const int half_step[8][4] = {
        {1, 0, 0, 0},
        {1, 1, 0, 0},
        {0, 1, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 1, 0},
        {0, 0, 1, 1},
        {0, 0, 0, 1},
        {1, 0, 0, 1}
    };

    static int phase = 0;
    phase = phase + direction & 7;
    for (int i = 0; i < COIL_PINS_SIZE; i++) {
        gpio_put(coil_pins[i], half_step[phase][i]);
    }
}
