#include "StateMachine.h"
#include "StepMotor.h"
#include "pico/time.h"

StateMachine::StateMachine()
    : stepMotor(coil_pins), current_state_(CurrentState::initial), left_limit(LIM_PIN_LEFT),
right_limit(LIM_PIN_RIGHT)
{
    stepMotor.init_coil_pins();
    left_limit.init();
    right_limit.init();
}

void StateMachine::run_sm() {
    switch (current_state_) {
        case CurrentState::initial: initial_st(); break;
        case CurrentState::calib: calib_st(); break;
        case CurrentState::idle: idle_st(); break;
        case CurrentState::run_motor: run_motor_st(); break;
        case CurrentState::open: open_st(); break;
        case CurrentState::close: close_st(); break;
        default: initial_st(); break;
    }
}

void StateMachine::next_state(const CurrentState s) {
    current_state_ = s;
}

void StateMachine::initial_st() {

}

void StateMachine::calib_st() {

}

void StateMachine::idle_st() {

}

void StateMachine::run_motor_st() {
    static int direction = 1;

    static bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit) {
        direction = 1;
        std::cout << "Change to open state.\n";
        next_state(CurrentState::open);
    }

    static bool right_hit = false;
    right_limit.detect_hit(right_hit, "Right");
    if (right_hit) {
        direction = -1;
        std::cout << "Change to close state.\n";
        next_state(CurrentState::close);
    }

    stepMotor.step(direction);
    sleep_ms(MOTOR_SLEEP_MS);
}

void StateMachine::open_st() {
    static bool right_hit = false;
    right_limit.detect_hit(right_hit, "Right");
    if (right_hit) {
        std::cout << "Change to idle state.\n";
        next_state(CurrentState::idle);
    }

    stepMotor.step(1);
    sleep_ms(MOTOR_SLEEP_MS);
}

void StateMachine::close_st() {
    static bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit) {
        std::cout << "Change to idle state.\n";
        next_state(CurrentState::idle);
    }

    stepMotor.step(-1);
    sleep_ms(MOTOR_SLEEP_MS);
}