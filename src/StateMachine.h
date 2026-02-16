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
    calib = 1,
    idle = 2,
    start_calib = 3,
    calib_open = 4,
    calib_close = 5,
    open = 6,
    close = 7
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
    [[nodiscard]] CurrentState currentState() const { return current_state_; }
    void update_position(int new_position);
private:
    StepMotor stepMotor;
    LimitSwitch left_limit;
    LimitSwitch right_limit;
    int position;
    std::optional<int> lowest_position;
    std::optional<int> highest_position;

    void initial_st();
    void idle_st();
    void start_calib_st();
    void calib_open_st();
    void calib_close_st();

    static void set_sm_state(SmState& sms, uint8_t value);
    static bool validateCurrentState(const SmState& sms);
    void writeCurrentState(uint8_t value);

    CurrentState current_state_{CurrentState::initial};
    bool last_ms_valid_{false};
    uint32_t last_ms_{0};
};

#endif