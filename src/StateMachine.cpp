#include "StateMachine.h"
#include "StepMotor.h"
#include "pico/time.h"

StateMachine::StateMachine()
    : stepMotor(coil_pins), left_limit(LIM_PIN_LEFT),
        right_limit(LIM_PIN_RIGHT), position(0), lowest_position(std::nullopt),
        highest_position(std::nullopt), current_state(CurrentState::initial)
{
    stepMotor.init_coil_pins();
    left_limit.init();
    right_limit.init();
}

void StateMachine::run_sm() {
    switch (current_state) {
        case CurrentState::initial: initial_st(); break;
        case CurrentState::idle: idle_st(); break;
        case CurrentState::start_calib: start_calib_st(); break;
        case CurrentState::calib_open: calib_open_st(); break;
        case CurrentState::calib_close: calib_close_st(); break;
        case CurrentState::open: open_st(); break;
        case CurrentState::close: close_st(); break;
        case CurrentState::step_correction: correction_st(); break;
        default: initial_st(); break;
    }
}

void StateMachine::next_state(const CurrentState s) {
    current_state = s;
}

void StateMachine::initial_st() {

}

void StateMachine::idle_st() {

}

void StateMachine::start_calib_st() {
    static int direction = -1;

    static bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit) {
        direction = 1;
        position = 0;
        std::cout << "Change to open state.\n";
        next_state(CurrentState::calib_open);
    }

    /*static bool right_hit = false;
    right_limit.detect_hit(right_hit, "Right");
    if (right_hit) {
        direction = -1;
        std::cout << "Change to close state.\n";
        next_state(CurrentState::calib_close);
    }*/

    stepMotor.step(direction);
    sleep_ms(MOTOR_SLEEP_MS);
}

void StateMachine::calib_open_st() {
    static bool right_hit = false;
    right_limit.detect_hit(right_hit, "Right");
    if (right_hit) {
        highest_position = position;
        if (lowest_position.has_value()) {
            std::cout << "Calibration completed.\n";
            std::cout << "Highest position: " << *highest_position << "\n";
            std::cout << "Lowest position: " << *lowest_position << "\n";
            door_status = 1;
            next_state(CurrentState::idle);
        }
        else {
            next_state(CurrentState::calib_close);
        }
    }

    stepMotor.step(1);
    sleep_ms(MOTOR_SLEEP_MS);
}

void StateMachine::calib_close_st() {
    static bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit) {
        if (lowest_position < 0) {
            *highest_position -= *lowest_position;
            lowest_position = 0;
        }
        else {
            lowest_position = position;
        }

        if (highest_position.has_value()) {
            std::cout << "Calibration completed.\n";
            std::cout << "Highest position: " << *highest_position << "\n";
            std::cout << "Lowest position: " << *lowest_position << "\n";
            door_status = 0;
            next_state(CurrentState::step_correction);
        }
        else {
            next_state(CurrentState::calib_open);
        }
    }

    stepMotor.step(-1);
    sleep_ms(MOTOR_SLEEP_MS);
}

void StateMachine::open_st() {
    static bool right_hit = false;
    right_limit.detect_hit(right_hit, "Right");
    if (right_hit) {
        door_status = 1;
        next_state(CurrentState::idle);
    }

    if (position >= *highest_position - 4) {
        door_status = 1;
        next_state(CurrentState::idle);
    }

    stepMotor.step(1);
    sleep_ms(MOTOR_SLEEP_MS);
}

void StateMachine::close_st() {
    static bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit) {
        door_status = 0;
        next_state(CurrentState::idle);
    }

    if (position <= *highest_position + 4) {
        door_status = 0;
        next_state(CurrentState::idle);
    }

    stepMotor.step(-1);
    sleep_ms(MOTOR_SLEEP_MS);
}

void StateMachine::correction_st() {
    if (position >= *lowest_position + 2) {
        next_state(CurrentState::idle);
    }
    else {
        stepMotor.step(1);
        sleep_ms(MOTOR_SLEEP_MS);
    }
}

CurrentState StateMachine::check_current_state() const {
    return current_state;
}

std::optional<int> StateMachine::get_door_status() const {
    return door_status;
}

void StateMachine::update_position(const int new_position) {
    position = new_position;
    std::cout << "Position: " << position << "\n";
}

int StateMachine::get_position() const {
    return position;
}

void StateMachine::reset_position() {
    position = 0;
}