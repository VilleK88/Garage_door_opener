#include "StateMachine.h"
#include "MqttService.h"
#include "StepMotor.h"
#include <iostream>
#include <array>
#include "pico/time.h"
#include "Eeprom.h"
#include "LedController.h"

StateMachine::StateMachine(MqttService& new_mqtt, LedController& newLedContr)
    : mqtt(new_mqtt), ledContr(newLedContr), left_limit(LIM_PIN_LEFT),
        right_limit(LIM_PIN_RIGHT)
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
    if (error) ledContr.set_brightness(LedController::LED1, LedController::LIGHT_OFF);
    const std::string str_st = get_st_string(st);
    std::cout << get_st_string(st) << "\n";
    mqtt.publish(MqttService::TOPIC_STAT, str_st.c_str(), 0, true);

    if (st == CurrentState::idle) {
        last_ms_valid_ = false;
        door_moving = false;
        eeprom.write_state16(Eeprom::STEP_POS_ADDR, motor_step_pos);
        eeprom.write_state(Eeprom::NEXT_DIR_ADDR, next_direction);
        eeprom.write_state(Eeprom::DOOR_MOV_ADDR, door_moving);
    }
    else {
        door_moving = true;
        eeprom.write_state(Eeprom::DOOR_MOV_ADDR, door_moving);
        last_encoder_change_ms = to_ms_since_boot(get_absolute_time());
    }

    if (st == CurrentState::init_calib) {
        calibrated = false;
        eeprom.write_state(Eeprom::CALIB_ADDR, calibrated);
    }

    current_state = st;
    last_ms_valid_ = false;
}

void StateMachine::idle_st() {

}

void StateMachine::init_calib_st() {
    bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit) {
        encoder_pos = 0;
        next_state(CurrentState::calib_open_door);
    }
    else {
        if (every_ms(step_ms))
            stepMotor.step(-step);
        check_if_stuck();
    }
}

void StateMachine::calib_open_door_st() {
    bool right_hit = false;
    right_limit.detect_hit(right_hit, "Right");
    if (right_hit) {
        highest_pos = motor_step_pos;
        next_state(CurrentState::calib_close_door);
    }
    else {
        if (every_ms(step_ms))
            motor_step_pos += stepMotor.run_step_motor(step);
        check_if_stuck();
    }
}

void StateMachine::calib_close_door_st() {
    bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit) {
        lowest_pos = motor_step_pos;

        std::cout << "Calibration completed.\n";
        std::cout << "Highest position: " << highest_pos << "\n";
        std::cout << "Lowest position: " << lowest_pos << "\n";
        std::cout << "Motor step position: " << motor_step_pos << "\n";
        next_state(CurrentState::step_correction);
    }
    else {
        if (every_ms(step_ms))
            motor_step_pos += stepMotor.run_step_motor(-step);
        check_if_stuck();
    }

}

void StateMachine::step_correction_st() {
    if (motor_step_pos >= lowest_pos + pos_offset) {
        next_direction = true;
        calibrated = true;
        error = false;
        eeprom.write_state16(Eeprom::STEP_POS_ADDR, motor_step_pos);
        eeprom.write_state16(Eeprom::LOWEST_POS_ADDR, lowest_pos);
        eeprom.write_state16(Eeprom::HIGHEST_POS_ADDR, highest_pos);
        eeprom.write_state(Eeprom::NEXT_DIR_ADDR, 1);
        eeprom.write_state(Eeprom::CALIB_ADDR, 1);
        std::cout << "Motor step position after correction: " << motor_step_pos << "\n";
        next_state(CurrentState::idle);
    }
    else {
        if (every_ms(step_ms))
            motor_step_pos += stepMotor.run_step_motor(step);
        check_if_stuck();
    }
}

void StateMachine::open_door_st() {
    bool right_hit = false;
    right_limit.detect_hit(right_hit, "Right");
    if (right_hit || motor_step_pos >= highest_pos - pos_offset) {
        next_direction = false;
        std::cout << "Motor step position: " << motor_step_pos << "\n";
        next_state(CurrentState::idle);
    }
    else {
        if (every_ms(step_ms))
            motor_step_pos += stepMotor.run_step_motor(step);
        check_if_stuck();
    }
}

void StateMachine::close_door_st() {
    bool left_hit = false;
    left_limit.detect_hit(left_hit, "Left");
    if (left_hit || motor_step_pos <= lowest_pos + pos_offset) {
        next_direction = true;
        eeprom.write_state(Eeprom::NEXT_DIR_ADDR, next_direction);
        eeprom.write_state16(Eeprom::STEP_POS_ADDR, motor_step_pos);
        std::cout << "Motor step position: " << motor_step_pos << "\n";
        next_state(CurrentState::idle);
    }
    else {
        if (every_ms(step_ms))
            motor_step_pos += stepMotor.run_step_motor(-step);
        check_if_stuck();
    }
}

void StateMachine::error_st() {
    if (every_ms(500)) {
        if (!ledContr.blink)
            ledContr.set_brightness(LedController::LED1, LedController::BR_MID);
        else
            ledContr.set_brightness(LedController::LED1, LedController::LIGHT_OFF);
        ledContr.blink = !ledContr.blink;
    }
}

void StateMachine::update_position(const int new_position) {
    if (new_position != encoder_pos) {
        encoder_pos = new_position;
        last_encoder_change_ms = to_ms_since_boot(get_absolute_time());
    }
}

void StateMachine::check_if_stuck() {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (door_moving && now - last_encoder_change_ms > fault_max_time_ms) {
        error = true;
        std::cout << "Recalibration required.\n";
        eeprom.write_state(Eeprom::CALIB_ADDR, 0);
        eeprom.write_state(Eeprom::DOOR_MOV_ADDR, 0);
        next_state(CurrentState::error_state);
    }
}

int StateMachine::get_position() const {
    return encoder_pos;
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
            std::cout << "Motor step position: " << motor_step_pos << "\n";
            next_state(CurrentState::idle);
        }
    }
    else {
        std::cout << "Calibrate first\n";
        mqtt.publish(MqttService::TOPIC_STAT, "Calibrate first", 0, true);
    }
}

void StateMachine::init_states() {
    constexpr std::array addresses = {
        Eeprom::CALIB_ADDR, Eeprom::NEXT_DIR_ADDR, Eeprom::STEP_POS_ADDR,
        Eeprom::LOWEST_POS_ADDR, Eeprom::HIGHEST_POS_ADDR
    };
    const std::array bool_states = {&calibrated, &next_direction};
    const std::array int_states = {&motor_step_pos, &lowest_pos, &highest_pos};
    const std::array str_states = {
        "Calibration", "Next direction",
        "Motor step position", "Lowest position", "Highest position"
    };

    Eeprom::GenSt gst_door_mov;
    const int door_mov_num = init_st(gst_door_mov, Eeprom::DOOR_MOV_ADDR, "Door moving");
    door_moving = door_mov_num;

    if(!door_moving) {
        int index = 0;
        for (auto& addr : addresses) {
            if (index < 2) {
                if (eeprom.validate_state(addr)) {
                    Eeprom::GenSt gst;
                    const int num = init_st(gst, addr, str_states[index]);
                    *bool_states[index] = num != 0;
                }
                else
                    std::cout << "State INVALID at addr: " << addr << "\n";
            }
            else if (calibrated) {
                if (eeprom.validate_state16(addr)) {
                    Eeprom::GenSt16 gst;
                    *int_states[index - 2] = init_st16(gst, addr, str_states[index]);
                }
                else
                    std::cout << "State16 INVALID at addr: " << addr << "\n";
            }
            index++;
        }
        std::cout << "Persistent states restored\n";
    }
    else {
        error = true;
        current_state = CurrentState::error_state;
        std::cout << "Power loss during motor operation resulted in an error state\n";
    }
}

int StateMachine::init_st(Eeprom::GenSt gst, const uint16_t addr, const std::string& str_st) const {
    eeprom.read_state(addr, &gst.state, &gst.not_state);
    //std::cout << str_st << " state loaded: " << static_cast<int>(gst.state) << "\n";
    return gst.state;
}

int StateMachine::init_st16(Eeprom::GenSt16 gst, const uint16_t addr, const std::string& str_st) const {
    eeprom.read_state16(addr, &gst.state, &gst.not_state);
    //std::cout << str_st << " state loaded: " << static_cast<int>(gst.state) << "\n";
    return gst.state;
}

std::string StateMachine::get_st_string(const CurrentState st) {
    switch (st) {
        case CurrentState::idle: return "Idle state";
        case CurrentState::init_calib: return "Initialize calibration state";
        case CurrentState::calib_open_door: return "Calibration open door state";
        case CurrentState::calib_close_door: return "Calibration close door state";
        case CurrentState::step_correction: return "Step correction state";
        case CurrentState::open_door: return "Open door state";
        case CurrentState::close_door: return "Close door state";
        case CurrentState::error_state: return "Error state";
        default: return "Idle state";
    }
}

bool StateMachine::every_ms(const uint32_t interval_ms) {
    const uint32_t now = to_ms_since_boot(get_absolute_time());
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