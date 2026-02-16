#include "SM.h"
#include "StepMotor.h"

SM::SM()
    : stepMotor(coil_pins),
      current_state_(CurrentState::initial)
{
    stepMotor.init_coil_pins();
}

void SM::run_sm() {
    switch (current_state_) {
        case CurrentState::initial: initial_st(); break;
        case CurrentState::calib: calib_st(); break;
        case CurrentState::idle: idle_st(); break;
        case CurrentState::run_motor: run_motor_st(); break;
        default: initial_st(); break;
    }
}

void SM::next_state(const CurrentState s) {
    current_state_ = s;
}

void SM::initial_st() {

}

void SM::calib_st() {

}

void SM::idle_st() {

}

void SM::run_motor_st() {
    stepMotor.run_step_motor(2, -1);
}