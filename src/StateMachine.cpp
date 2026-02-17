#include "StateMachine.h"
#include "StepMotor.h"
#include "pico/time.h"

StateMachine::StateMachine()
    : stepMotor(coil_pins), left_limit(LIM_PIN_LEFT),
        right_limit(LIM_PIN_RIGHT), position(0), lowest_position(0),
        highest_position(0), calib_status(false), open_door(false), current_state(CurrentState::initial)
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

void StateMachine::next_state(const CurrentState s, const std::string& st_text) {
    std::cout << st_text << "\n";
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
        next_state(CurrentState::calib_open, "Calibration open door state");
    }

    stepMotor.step(direction);
    sleep_ms(MOTOR_SLEEP_MS);
}

void StateMachine::calib_open_st() {
    static bool right_hit = false;
    right_limit.detect_hit(right_hit, "Right");
    if (right_hit) {
        highest_position = position;
        /*if (lowest_position.has_value()) {
            std::cout << "Calibration completed.\n";
            std::cout << "Highest position: " << *highest_position << "\n";
            std::cout << "Lowest position: " << *lowest_position << "\n";
            door_status = 1;
            next_state(CurrentState::idle);
        }
        else {
            next_state(CurrentState::calib_close);
        }*/
        next_state(CurrentState::calib_close, "Calibration close door state");
    }

    stepMotor.step(1);
    sleep_ms(MOTOR_SLEEP_MS);
}

void StateMachine::calib_close_st() {
    static bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit) {
        if (lowest_position < 0) {
            std::cout << "Highest position: " << highest_position << "\n";
            std::cout << "Lowest position: " << lowest_position << "\n";
            highest_position -= lowest_position;
            lowest_position = 0;
        }
        else {
            lowest_position = position;
        }

        std::cout << "Calibration completed.\n";
        std::cout << "Highest position: " << highest_position << "\n";
        std::cout << "Lowest position: " << lowest_position << "\n";
        open_door = true;
        next_state(CurrentState::step_correction, "Step correction state");
    }

    stepMotor.step(-1);
    sleep_ms(MOTOR_SLEEP_MS);
}

void StateMachine::open_st() {
    static bool right_hit = false;
    right_limit.detect_hit(right_hit, "Right");
    if (right_hit || position >= highest_position - 2) {
        open_door = false;
        next_state(CurrentState::idle, "Idle state");
    }

    stepMotor.step(1);
    sleep_ms(MOTOR_SLEEP_MS);
}

void StateMachine::close_st() {
    static bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit || position <= lowest_position + 2) {
        open_door = true;
        next_state(CurrentState::idle, "Idle state");
    }

    stepMotor.step(-1);
    sleep_ms(MOTOR_SLEEP_MS);
}

void StateMachine::correction_st() {
    if (position >= lowest_position + 2) {
        calib_status = true;
        next_state(CurrentState::idle, "Idle state");
    }
    else {
        stepMotor.step(1);
        sleep_ms(MOTOR_SLEEP_MS);
    }
}

CurrentState StateMachine::check_st() const {
    return current_state;
}

bool StateMachine::check_calib_status() const {
    return calib_status;
}

// Should door be closing or opening
bool StateMachine::get_door_status() const {
    return open_door;
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