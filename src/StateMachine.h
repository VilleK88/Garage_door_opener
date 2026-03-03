#ifndef SM_H
#define SM_H
#pragma once
#include <iostream>
#include "Eeprom.h"
#include "StepMotor.h"
#include "LimitSwitch.h"
#include "LedController.h"

enum class CurrentState : uint8_t {
    idle = 0,
    init_calib = 1,
    calib_open_door = 2,
    calib_close_door = 3,
    step_correction = 4,
    open_door = 5,
    close_door = 6,
    error_state = 7
};

struct SmState {
    uint8_t state;
    uint8_t not_state;
};

struct MainSmState {
    uint8_t state;
};

class MqttService;
class LedController;

class StateMachine final {
public:
    //explicit StateMachine(MqttService& new_mqtt, LedController& newLedContr);
    explicit StateMachine(MqttService& new_mqtt, LedController& newLedContr);
    void run_sm();
    void next_state(CurrentState st);
    [[nodiscard]] CurrentState currentState() const { return current_state; }
    void update_position(int new_position);
    [[nodiscard]] int get_position() const;
    void handle_door();

private:
    StepMotor stepMotor;
    Eeprom eeprom;
    MqttService& mqtt;
    LedController& ledContr;
    LimitSwitch left_limit;
    LimitSwitch right_limit;
    bool door_moving{false};
    bool calibrated{false};
    bool next_direction{false}; // if door should be closing or opening
    bool error{false};

    int motor_step_pos{0};
    int lowest_pos{0};
    int highest_pos{0};

    int encoder_pos{0};
    int previous_encoder_pos{0};
    uint32_t last_encoder_change_ms{0};
    int fault_max_time_ms{1000};

    void idle_st();
    void init_calib_st();
    void calib_open_door_st();
    void calib_close_door_st();
    void step_correction_st();
    void open_door_st();
    void close_door_st();
    void error_st();

    void init_states();
    [[nodiscard]] int init_st(Eeprom::GenSt gst, uint16_t addr, const std::string& str_st) const;
    [[nodiscard]] int init_st16(Eeprom::GenSt16 gst, uint16_t addr, const std::string& str_st) const;

    std::string get_st_string(CurrentState st);

    void check_if_stuck();
    bool every_ms(uint32_t interval_ms);

    using Handler = void (StateMachine::*)();
    static constexpr Handler handlers[] = {
        &StateMachine::idle_st,
        &StateMachine::init_calib_st,
        &StateMachine::calib_open_door_st,
        &StateMachine::calib_close_door_st,
        &StateMachine::step_correction_st,
        &StateMachine::open_door_st,
        &StateMachine::close_door_st,
        &StateMachine::error_st
    };

    CurrentState current_state{CurrentState::idle};

    bool last_ms_valid_{false};
    uint32_t last_ms_{0};
};

#endif