#include "StateMachine.h"
#include "StepMotor.h"
#include <iostream>
#include <array>
#include "pico/time.h"
#include "Eeprom.h"

StateMachine::StateMachine()
    : stepMotor(coil_pins), left_limit(LIM_PIN_LEFT), right_limit(LIM_PIN_RIGHT),
        calibrated(false), door_moving(false), next_direction(false),
        position(0), lowest_position(0), highest_position(0)
{
    std::cout << "Boot\n";
    stepMotor.init_coil_pins();
    eeprom.init_eeprom();
    init_states();
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

void StateMachine::next_state(const CurrentState st) {
    std::cout << get_st_string(st) << "\n";

    if (st != CurrentState::idle) {
        door_moving = true;
        eeprom.write_state(Eeprom::DOOR_MOV_ADDR, 1);
    }
    else {
        door_moving = false;
        eeprom.write_state(Eeprom::DOOR_MOV_ADDR, 0);
    }
    if (st == CurrentState::init_calib) {
        calibrated = false;
        eeprom.write_state(Eeprom::CALIB_ADDR, 0);
    }

    current_state = st;
    last_ms_valid_ = false;
}

void StateMachine::initial_st() {

}

void StateMachine::idle_st() {

}

void StateMachine::init_calib_st() {
    bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit) {
        position = 0;
        next_state(CurrentState::calib_open_door);
    }
    else {
        if (every_ms(1))
            stepMotor.step(-1);
    }
}

void StateMachine::calib_open_door_st() {
    bool right_hit = false;
    right_limit.detect_hit(right_hit, "Right");
    if (right_hit) {
        highest_position = position;
        next_state(CurrentState::calib_close_door);
    }
    else {
        if (every_ms(1))
            stepMotor.step(1);
    }
}

void StateMachine::calib_close_door_st() {
    bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit) {
        position = 0;
        lowest_position = 0;

        std::cout << "Calibration completed.\n";
        std::cout << "Highest position: " << highest_position << "\n";
        std::cout << "Lowest position: " << lowest_position << "\n";
        next_state(CurrentState::step_correction);
    }
    else {
        if (every_ms(1))
            stepMotor.step(-1);
    }

}

void StateMachine::step_correction_st() {
    if (position >= lowest_position + 2) {
        next_direction = true;
        calibrated = true;
        eeprom.write_state16(Eeprom::POS_ADDR, position);
        eeprom.write_state16(Eeprom::LOWEST_POS_ADDR, lowest_position);
        eeprom.write_state16(Eeprom::HIGHEST_POS_ADDR, highest_position);
        eeprom.write_state(Eeprom::NEXT_DIR_ADDR, 1);
        eeprom.write_state(Eeprom::CALIB_ADDR, 1);
        next_state(CurrentState::idle);
    }
    else {
        if (every_ms(1))
            stepMotor.step(1);
    }
}

void StateMachine::open_door_st() {
    bool right_hit = false;
    right_limit.detect_hit(right_hit, "Right");
    if (right_hit || position >= highest_position - 1) {
        next_direction = false;
        eeprom.write_state(Eeprom::NEXT_DIR_ADDR, 0);
        eeprom.write_state16(Eeprom::POS_ADDR, position);
        next_state(CurrentState::idle);
    }
    else {
        if (every_ms(1))
            stepMotor.step(1);
    }
}

void StateMachine::close_door_st() {
    bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit || position <= lowest_position + 2) {
        next_direction = true;
        eeprom.write_state(Eeprom::NEXT_DIR_ADDR, 1);
        eeprom.write_state16(Eeprom::POS_ADDR, position);
        next_state(CurrentState::idle);
    }
    else {
        if (every_ms(1))
            stepMotor.step(-1);
    }
}

void StateMachine::update_position(const int new_position) {
    position = new_position;
}

int StateMachine::get_position() const {
    return position;
}

void StateMachine::handle_door() {
    if (calibrated) {
        if (!door_moving) {
            if (!next_direction)
                next_state(CurrentState::close_door);
            else
                next_state(CurrentState::open_door);
        }
        else {
            next_direction = !next_direction;
            eeprom.write_state(Eeprom::NEXT_DIR_ADDR, next_direction);
            eeprom.write_state16(Eeprom::POS_ADDR, position);
            next_state(CurrentState::idle);
        }
    }
}

void StateMachine::init_states() {
    const std::array bool_states = {&calibrated, &door_moving, &next_direction};
    const std::array int_states = {&position, &lowest_position, &highest_position};

    int index = 0;
    for (auto& addr : Eeprom::STATE_ADDRESSES) {
        if (index < 3) {
            if (eeprom.validate_state(addr)) {
                Eeprom::GenSt gst;
                const int num = init_st(gst, addr);
                *bool_states[index] = num != 0;
            }
            else
                std::cout << "State INVALID at addr: " << addr << "\n";
        }
        else if (calibrated && !door_moving) {
            if (eeprom.validate_state16(addr)) {
                Eeprom::GenSt16 gst;
                *int_states[index - 3] = init_st16(gst, addr);
            }
            else
                std::cout << "State16 INVALID at addr: " << addr << "\n";
        }
        index++;
    }
}

int StateMachine::init_st(Eeprom::GenSt gst, const uint16_t addr) const {
    eeprom.read_state(addr, &gst.state, &gst.not_state);
    std::cout << "State downloaded: " << static_cast<int>(gst.state) << "\n";
    return gst.state;
}

int StateMachine::init_st16(Eeprom::GenSt16 gst, const uint16_t addr) const {
    eeprom.read_state16(addr, &gst.state, &gst.not_state);
    std::cout << "State downloaded: " << static_cast<int>(gst.state) << "\n";
    return gst.state;
}

std::string StateMachine::get_st_string(CurrentState st) {
    switch (st) {
        case CurrentState::idle: return "Idle state";
        case CurrentState::init_calib: return "Initialize calibration state";
        case CurrentState::calib_open_door: return "Calibration open door state";
        case CurrentState::calib_close_door: return "Calibration close door state";
        case CurrentState::step_correction: return "Step correction state";
        case CurrentState::open_door: return "Open door state";
        case CurrentState::close_door: return "Close door state";
        case CurrentState::stop_door: return "Stop door state";
        default: return "Idle state";
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