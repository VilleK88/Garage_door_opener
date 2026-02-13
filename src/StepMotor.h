#ifndef STEPMOTOR_H
#define STEPMOTOR_H
#include <iostream>
#include <array>
#include "pico/types.h"

#define IN1 2
#define IN2 3
#define IN3 6
#define IN4 13
#define COIL_PINS_SIZE 4
// Array grouping all coil control pins for iteration
static constexpr int coil_pins[] = {IN1, IN2, IN3, IN4};

class StepMotor {
public:
    explicit StepMotor(const std::array<uint, 4>& pins);
    void init_coil_pins() const;
    void step(int direction) const;
private:
    std::array<uint, 4> coil_pins;
};


#endif