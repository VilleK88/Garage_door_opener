#include "StateMachine.h"
#include "StepMotor.h"
#include "pico/time.h"

StateMachine::StateMachine()
    : stepMotor(coil_pins), left_limit(LIM_PIN_LEFT),
        right_limit(LIM_PIN_RIGHT), position(0), lowest_position(0),
        highest_position(0), calibrated(false), next_direction(false),
        door_moving(false), current_state(CurrentState::initial)
{
    stepMotor.init_coil_pins();
    left_limit.init();
    right_limit.init();
}

void StateMachine::run_sm() {
    const auto states = static_cast<uint8_t>(current_state);
    if (states >= std::size(handlers))
        (this->*handlers[0])();
    else
        (this->*handlers[states])();
}

void StateMachine::next_state(const CurrentState st, const std::string& st_text) {
    std::cout << st_text << "\n";

    if (st != CurrentState::initial && st != CurrentState::idle)
        door_moving = true;
    else
        door_moving = false;

    current_state = st;
    last_ms_valid_ = false;
}

void StateMachine::initial_st() {

}

void StateMachine::idle_st() {

}

void StateMachine::calib_st() {
    bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit) {
        position = 0;
        std::cout << "Change to open state.\n";
        next_state(CurrentState::calib_open, "Calibration open door state");
    }
    else {
        if (every_ms(1))
            stepMotor.step(-1);
    }
}

void StateMachine::calib_open_st() {
    bool right_hit = false;
    right_limit.detect_hit(right_hit, "Right");
    if (right_hit) {
        highest_position = position;
        next_state(CurrentState::calib_close, "Calibration close door state");
    }
    else {
        if (every_ms(1))
            stepMotor.step(1);
    }
}

void StateMachine::calib_close_st() {
    bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit) {
        position = 0;
        lowest_position = 0;

        std::cout << "Calibration completed.\n";
        std::cout << "Highest position: " << highest_position << "\n";
        std::cout << "Lowest position: " << lowest_position << "\n";
        next_direction = true;
        next_state(CurrentState::step_correction, "Step correction state");
    }
    else {
        if (every_ms(1))
            stepMotor.step(-1);
    }

}

void StateMachine::open_st() {
    bool right_hit = false;
    right_limit.detect_hit(right_hit, "Right");
    if (right_hit || position >= highest_position - 1) {
        next_direction = false;
        next_state(CurrentState::idle, "Idle state");
    }
    else {
        if (every_ms(1))
            stepMotor.step(1);
    }
}

void StateMachine::close_st() {
    bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit || position <= lowest_position + 2) {
        next_direction = true;
        next_state(CurrentState::idle, "Idle state");
    }
    else {
        if (every_ms(1))
            stepMotor.step(-1);
    }
}

void StateMachine::correction_st() {
    if (position >= lowest_position + 2) {
        calibrated = true;
        next_state(CurrentState::idle, "Idle state");
    }
    else {
        if (every_ms(1))
            stepMotor.step(1);
    }
}

CurrentState StateMachine::check_st() const {
    return current_state;
}

/*bool StateMachine::check_calib_status() const {
    return calibrated;
}*/

// Should door be closing or opening
/*bool StateMachine::get_door_status() const {
    return next_direction_door;
}*/

void StateMachine::update_position(const int new_position) {
    position = new_position;
    std::cout << "Position: " << position << "\n";
}

int StateMachine::get_position() const {
    return position;
}

/*void StateMachine::reset_position() {
    position = 0;
}*/

/*void StateMachine::change_door_moving_status(bool is_door_moving) {
    door_moving = is_door_moving;
}*/

/*bool StateMachine::get_door_moving_status() const {
    return door_moving;
}*/

void StateMachine::start_calibration() {
    calibrated = false;
    next_state(CurrentState::start_calib, "Calibration state");
}

void StateMachine::handle_door() {
    if (calibrated) {
        if (!door_moving) {
            if (!next_direction)
                next_state(CurrentState::close, "Close door state");
            else
                next_state(CurrentState::open, "Open door state");
        }
        else {
            next_direction = !next_direction;
            next_state(CurrentState::idle, "Idle state");
        }
    }
}

bool StateMachine::every_ms(uint32_t interval_ms) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (!last_ms_valid_) {
        last_ms_ = now;
        last_ms_valid_ = true;
    }

    if (now - last_ms_ >= interval_ms) {
        last_ms_ = now;
        return true;
    }

    return false;
}