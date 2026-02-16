#ifndef STATEMACHINE_H
#define STATEMACHINE_H
#pragma once
#include <cstdint>
#include <iostream>
#include <vector>
#include <array>

enum class CurrentState : uint8_t {
    initial = 0,
    calib = 1,
    idle = 2,
    run_motor = 3
};

struct SmState {
    uint8_t state;
    uint8_t not_state;
};

struct MainSmState {
    uint8_t state;
};

class SM final {
public:
    SM();
    void run();

    void nextState(CurrentState s);
    bool checkState(CurrentState s) const;
    bool canPress() const;

    CurrentState currentState() const { return current_state_; }

private:
    void initial_st();
    void calib_st();
    void idle_st();
    void run_motor_st();

    static void set_sm_state(SmState& sms, uint8_t value);
    static bool validateCurrentState(const SmState& sms);
    void writeCurrentState(uint8_t value);

    CurrentState current_state_{CurrentState::initial};
    bool last_ms_valid_{false};
    uint32_t last_ms_{0};
};


#endif