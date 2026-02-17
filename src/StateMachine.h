#ifndef SM_H
#define SM_H
#pragma once
#include <cstdint>
#include <iostream>
#include <vector>
#include <array>
#include <optional>

#include "StepMotor.h"
#include "LimitSwitch.h"

enum class CurrentState : uint8_t {
    initial = 0,
    idle = 1,
    start_calib = 2,
    calib_open = 3,
    calib_close = 4,
    open = 5,
    close = 6,
    step_correction = 7,
    stop_door
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
    void next_state(CurrentState st, const std::string& st_text);
    [[nodiscard]] CurrentState currentState() const { return current_state; }
    [[nodiscard]] CurrentState check_st() const;
    //[[nodiscard]] bool check_calib_status() const;
    //[[nodiscard]] bool get_door_status() const;
    void update_position(int new_position);
    [[nodiscard]] int get_position() const;
    //void reset_position();
    //void change_door_moving_status(bool is_door_moving);
    //bool get_door_moving_status() const;
    void start_calibration();
    void handle_door();
private:
    StepMotor stepMotor;
    LimitSwitch left_limit;
    LimitSwitch right_limit;
    int position;
    int lowest_position;
    int highest_position;
    bool calibrated;
    bool next_direction; // if door should be closing or opening
    bool door_moving;

    void initial_st();
    void idle_st();
    void calib_st();
    void calib_open_st();
    void calib_close_st();
    void open_st();
    void close_st();
    void correction_st();

    bool every_ms(uint32_t interval_ms);

    //static void set_sm_state(SmState& sms, uint8_t value);
    //static bool validateCurrentState(const SmState& sms);
    //void writeCurrentState(uint8_t value);

    using Handler = void (StateMachine::*)();
    static constexpr Handler handlers[] = {
        &StateMachine::initial_st,
        &StateMachine::idle_st,
        &StateMachine::calib_st,
        &StateMachine::calib_open_st,
        &StateMachine::calib_close_st,
        &StateMachine::open_st,
        &StateMachine::close_st,
        &StateMachine::correction_st
    };

    CurrentState current_state{CurrentState::initial};
    bool last_ms_valid_{false};
    uint32_t last_ms_{0};
};

#endif