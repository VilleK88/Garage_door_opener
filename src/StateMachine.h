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
    step_correction = 7
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
    void next_state(CurrentState s);
    [[nodiscard]] CurrentState currentState() const { return current_state; }
    CurrentState check_current_state() const;
    std::optional<int> get_door_status() const;
    void update_position(int new_position);
    [[nodiscard]] int get_position() const;
    void reset_position();
private:
    StepMotor stepMotor;
    LimitSwitch left_limit;
    LimitSwitch right_limit;
    int position;
    std::optional<int> lowest_position;
    std::optional<int> highest_position;
    std::optional<int> door_status;

    void initial_st();
    void idle_st();
    void start_calib_st();
    void calib_open_st();
    void calib_close_st();
    void open_st();
    void close_st();
    void correction_st();

    static void set_sm_state(SmState& sms, uint8_t value);
    static bool validateCurrentState(const SmState& sms);
    void writeCurrentState(uint8_t value);

    CurrentState current_state{CurrentState::initial};
    bool last_ms_valid_{false};
    uint32_t last_ms_{0};
};

#endif