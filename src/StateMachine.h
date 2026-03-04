#ifndef SM_H
#define SM_H
#pragma once
#include <iostream>
#include "Eeprom.h"
#include "StepMotor.h"
#include "LimitSwitch.h"
#include "LedController.h"

// Enumeration describing all possible states of the garage door controller
enum class CurrentState : uint8_t {
    idle = 0, // System idle, waiting for command
    init_calib = 1, // Start calibration procedure
    calib_open_door = 2, // Move door fully open during calibration
    calib_close_door = 3, // Move door fully closed during calibration
    step_correction = 4, // Correct step offset after calibration
    open_door = 5, // Opening the door
    close_door = 6, // Closing the door
    error_state = 7 // Error state (motor fault, etc.)
};

// Structure used to store 8-bit persistent state values with redundancy
struct SmState {
    uint8_t state;
    uint8_t not_state;
};

// Structure used for storing main state information
struct MainSmState {
    uint8_t state;
};

// Forward declarations
class MqttService;
class LedController;

// Main application state machine controlling the garage door
class StateMachine final {
public:
    // Constructor requires MQTT service and LED controller references
    explicit StateMachine(MqttService& new_mqtt, LedController& newLedContr);
    // Executes the current state's handler
    void run_sm();
    // Transition to a new state
    void next_state(CurrentState st);
    // Returns the current state
    [[nodiscard]] CurrentState currentState() const { return current_state; }
    // Update encoder position (called from rotary encoder event)
    void update_position(int new_position);
    // Get current encoder position
    [[nodiscard]] int get_position() const;
    // Handles door open/close toggle logic
    void handle_door();

private:
    // Hardware controllers
    StepMotor stepMotor;
    Eeprom eeprom;
    MqttService& mqtt;
    LedController& ledContr;
    LimitSwitch left_limit;
    LimitSwitch right_limit;

    // Runtime flags
    bool door_moving{false}; // true while motor is running
    bool calibrated{false}; // true if door calibration completed
    bool next_direction{false}; // next direction (open/close)
    bool error{false}; // error flag

    // Motor step positions
    int motor_step_pos{0};
    int lowest_pos{0};
    int highest_pos{0};
    int pos_offset{500}; // safety margin near limits

    // Step timing parameters
    int step_ms{1};
    int step{1};

    // Encoder monitoring for fault detection
    int encoder_pos{0};
    int previous_encoder_pos{0};
    uint32_t last_encoder_change_ms{0};
    int fault_max_time_ms{1000};

    // State handlers
    void idle_st();
    void init_calib_st();
    void calib_open_door_st();
    void calib_close_door_st();
    void step_correction_st();
    void open_door_st();
    void close_door_st();
    void error_st();

    // Restore persistent states from EEPROM
    void init_states();

    // Helpers for reading persistent states
    [[nodiscard]] int init_st(Eeprom::GenSt gst, uint16_t addr, const std::string& str_st) const;
    [[nodiscard]] int init_st16(Eeprom::GenSt16 gst, uint16_t addr, const std::string& str_st) const;

    // Convert state enum to human-readable string
    static std::string get_st_string(CurrentState st);

    // Detect if motor is stuck
    void check_if_stuck();

    // Non-blocking periodic timer helper
    bool every_ms(uint32_t interval_ms);

    // Pointer-to-member state handler type
    using Handler = void (StateMachine::*)();

    // Table of state handler functions
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

    // Current state
    CurrentState current_state{CurrentState::idle};

    // Timing variables used by every_ms()
    bool last_ms_valid_{false};
    uint32_t last_ms_{0};

    void print_calib_info() const;
};

#endif