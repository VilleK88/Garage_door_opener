#ifndef SM_H
#define SM_H
#pragma once
#include <cstdint>
#include <iostream>
#include <vector>
#include <array>
#include <optional>

#include "Eeprom.h"
#include "StepMotor.h"
#include "LimitSwitch.h"

enum class CurrentState : uint8_t {
    idle = 0,
    init_calib = 1,
    calib_open_door = 2,
    calib_close_door = 3,
    step_correction = 4,
    open_door = 5,
    close_door = 6,
    stop_door = 7
};

struct SmState {
    uint8_t state;
    uint8_t not_state;
};

struct MainSmState {
    uint8_t state;
};

class StateMachine final {
public:
    StateMachine();
    void run_sm();
    void next_state(CurrentState st);
    [[nodiscard]] CurrentState currentState() const { return current_state; }
    //[[nodiscard]] CurrentState check_st() const;
    void update_position(int new_position);
    [[nodiscard]] int get_position() const;
    void handle_door();
private:
    StepMotor stepMotor;
    Eeprom eeprom;
    LimitSwitch left_limit;
    LimitSwitch right_limit;
    bool calibrated;
    bool door_moving;
    bool next_direction; // if door should be closing or opening
    int position;
    int lowest_position;
    int highest_position;

    void initial_st();
    void idle_st();
    void init_calib_st();
    void calib_open_door_st();
    void calib_close_door_st();
    void step_correction_st();
    void open_door_st();
    void close_door_st();

    void init_states();
    [[nodiscard]] int init_st(Eeprom::GenSt gst, uint16_t addr) const;
    [[nodiscard]] int init_st16(Eeprom::GenSt16 gst, uint16_t addr) const;

    std::string get_st_string(CurrentState st);

    bool every_ms(uint32_t interval_ms);

    using Handler = void (StateMachine::*)();
    static constexpr Handler handlers[] = {
        &StateMachine::idle_st,
        &StateMachine::init_calib_st,
        &StateMachine::calib_open_door_st,
        &StateMachine::calib_close_door_st,
        &StateMachine::step_correction_st,
        &StateMachine::open_door_st,
        &StateMachine::close_door_st
    };

    CurrentState current_state{CurrentState::idle};

    bool last_ms_valid_{false};
    uint32_t last_ms_{0};
};

#endif