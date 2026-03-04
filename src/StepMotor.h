#ifndef STEPMOTOR_H
#define STEPMOTOR_H
#include <iostream>
#include <array>
#include "pico/types.h"
//#define MOTOR_SLEEP_MS 1

class StepMotor {
public:
    explicit StepMotor();
    void init_coil_pins() const;
    void step(int direction) const;
    [[nodiscard]] int run_step_motor(int direction) const;
private:
    uint IN1 = 2;
    uint IN2 = 3;
    uint IN3 = 6;
    uint IN4 = 13;
    std::array<uint, 4> coil_pins = {IN1, IN2, IN3, IN4};
    int COIL_PINS_SIZE = 4;
    //int MOTOR_SLEEP_MS{1};
};

#endif