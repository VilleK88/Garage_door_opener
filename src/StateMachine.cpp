#include "StateMachine.h"
#include "MqttService.h"
#include "StepMotor.h"
#include <iostream>
#include <array>
#include "pico/time.h"
#include "Eeprom.h"
#include "LedController.h"

// Constructor: initialize references, hardware modules, and restore persistent state.
StateMachine::StateMachine(MqttService& new_mqtt, LedController& newLedContr)
    : mqtt(new_mqtt), ledContr(newLedContr), left_limit(LimitSwitch::LIM_PIN_LEFT),
        right_limit(LimitSwitch::LIM_PIN_RIGHT)
{
    std::cout << "Boot\n";
    // Initialize motor GPIOs and EEPROM storage
    stepMotor.init_coil_pins();
    eeprom.init_eeprom();
    // Restore calibration/motion state from EEPROM (or enter error state on unsafe restore)
    init_states();
    // Initialize limit switch inputs
    left_limit.init();
    right_limit.init();
}

// State-machine dispatcher: calls handler based on current_state.
void StateMachine::run_sm() {
    const auto states = static_cast<uint8_t>(current_state);
    // Safety: if state enum is out of range, fall back to idle handler.
    if (states >= std::size(handlers))
        (this->*handlers[0])();
    else
        (this->*handlers[states])();
}

// Transition helper: handles bookkeeping, persistence, and status publishing.
void StateMachine::next_state(const CurrentState st) {
    // Convert state to readable string, print it, and publish retained status to MQTT.
    const std::string str_st = get_st_string(st);
    std::cout << get_st_string(st) << "\n";
    mqtt.publish(MqttService::TOPIC_STAT, str_st.c_str(), 0, true);

    if (st == CurrentState::calib_open_door) {
        // Starting calibration invalidates previous calibration state.
        calibrated = false;
        ledContr.set_brightness(LedController::LED1, LedController::LIGHT_OFF);
        eeprom.write_state(Eeprom::CALIB_ADDR, calibrated);
    }
    
    // When entering idle, mark motion complete and persist final state/position.
    if (st == CurrentState::idle || st == CurrentState::error) {
        door_moving = false;
        // Persist key values so power cycles restore safely.
        eeprom.write_state16(Eeprom::STEP_POS_ADDR, motor_step_pos);
        eeprom.write_state(Eeprom::NEXT_DIR_ADDR, next_direction);
        eeprom.write_state(Eeprom::DOOR_MOV_ADDR, door_moving);
    }
    // When entering a non-idle movement state, mark door as moving and start stall timer.
    else {
        door_moving = true;
        eeprom.write_state(Eeprom::DOOR_MOV_ADDR, door_moving);
        last_encoder_change_ms = to_ms_since_boot(get_absolute_time());
    }

    // Commit the state transition and reset periodic timer helper.
    last_ms_valid_ = false;
    current_state = st;
}

// Idle state: no motor action. Waits for commands/events.
void StateMachine::idle_st() {
    sleep_ms(1);
}

// Calibration: move toward the right limit.
void StateMachine::calib_open_door_st() {
    // Right limit reached: record maximum position and move to closing calibration.
    if (right_limit.detect_hit("Right")) {
        motor_step_pos = 0;
        next_state(CurrentState::calib_close_door);
    }
    else {
        // Step motor toward open direction and update internal step position.
        if (every_ms(step_ms))
            motor_step_pos += stepMotor.run_step_motor(step);
        // Detect stall condition.
        check_if_stuck();
    }
}

// Calibration: move back toward the left limit.
void StateMachine::calib_close_door_st() {
    // Left limit reached: record minimum position and proceed to step correction.
    if (left_limit.detect_hit("Left")) {
        if (motor_step_pos < 0) motor_step_pos = -motor_step_pos;
        highest_pos = motor_step_pos;
        motor_step_pos = 0;
        encoder_pos = 0;
        lowest_pos = 0;

        print_calib_info();

        // After correction, set direction for next move and mark calibration complete.
        //error = false;
        // Persist calibration results and current state.
        eeprom.write_state16(Eeprom::STEP_POS_ADDR, motor_step_pos);
        eeprom.write_state16(Eeprom::LOWEST_POS_ADDR, lowest_pos);
        eeprom.write_state16(Eeprom::HIGHEST_POS_ADDR, highest_pos);
        next_direction = true;
        eeprom.write_state(Eeprom::NEXT_DIR_ADDR, next_direction);
        calibrated = true;
        eeprom.write_state(Eeprom::CALIB_ADDR, calibrated);
        std::cout << "Motor step position after correction: " << motor_step_pos << "\n";
        // Return to idle ready for operation.
        next_state(CurrentState::idle);
    }
    else {
        // Step motor toward close direction and update internal step position.
        if (every_ms(step_ms))
            motor_step_pos += stepMotor.run_step_motor(-step);
        // Detect stall condition.
        check_if_stuck();
    }

}

// Opening state: move toward the right limit until limit/margin is reached.
void StateMachine::open_door_st() {
    // Stop opening if we reached max margin.
    if (motor_step_pos >= highest_pos) {
        next_direction = false; // next operation should be closing
        std::cout << "Motor step position: " << motor_step_pos << "\n";
        next_state(CurrentState::idle);
    }
    else {
        // Step motor toward open direction.
        if (every_ms(step_ms))
            motor_step_pos += stepMotor.run_step_motor(step);
        // Detect stall condition.
        check_if_stuck();
        if (motor_step_pos > highest_pos) {
            std::cout << "Motor step position out off bounds: " << motor_step_pos << "\n";
            to_error_st();
        }
    }
}

// Closing state: move toward the left limit until limit/margin is reached.
void StateMachine::close_door_st() {
    // Stop closing if we reached min margin.
    if (motor_step_pos <= lowest_pos) {
        next_direction = true; // next operation should be opening
        std::cout << "Motor step position: " << motor_step_pos << "\n";
        next_state(CurrentState::idle);
    }
    else {
        // Step motor toward close direction.
        if (every_ms(step_ms))
            motor_step_pos += stepMotor.run_step_motor(-step);
        // Detect stall condition.
        check_if_stuck();
        if (motor_step_pos < lowest_pos) {
            std::cout << "Motor step position out off bounds:: " << motor_step_pos << "\n";
            to_error_st();
        }
    }
}

// Error state: blink an LED to indicate fault condition.
// Uses non-blocking timing via every_ms().
void StateMachine::error_st() {
    if (every_ms(LedController::blink_time_ms)) {
        if (!ledContr.blink)
            ledContr.set_brightness(LedController::LED1, LedController::BR_MID);
        else
            ledContr.set_brightness(LedController::LED1, LedController::LIGHT_OFF);
        // Toggle blink state each interval.
        ledContr.blink = !ledContr.blink;
    }
}

// Update encoder position (called when rotary encoder event changes).
// Also refreshes stall detection timer when movement is detected.
void StateMachine::update_position(const int new_position) {
    if (new_position != encoder_pos) {
        encoder_pos = new_position;
        last_encoder_change_ms = to_ms_since_boot(get_absolute_time());
    }
}

// Stall detection: if motor is commanded to move but encoder has not changed
// for fault_max_time_ms, enter error state and require recalibration.
void StateMachine::check_if_stuck() {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (door_moving && now - last_encoder_change_ms > fault_max_time_ms)
        to_error_st();
}

// Getter for current encoder position.
int StateMachine::get_position() const {
    return encoder_pos;
}

// Main button/command handler for door movement.
// Behavior:
// - If not calibrated: reject and publish message.
// - If idle: start opening/closing depending on next_direction.
// - If moving: stop motor (go idle) and flip next_direction for next time.
void StateMachine::handle_door() {
    if (calibrated) {
        if (!door_moving) {
            // Start a move from idle.
            if (!next_direction)
                next_state(CurrentState::close_door);
            else
                next_state(CurrentState::open_door);
        }
        else {
            // Door is moving: treat command as "stop" and invert next direction.
            next_direction = !next_direction;
            next_state(CurrentState::idle);
        }
    }
    else {
        // Calibration must be done before normal operation.
        std::cout << "Calibrate first\n";
        mqtt.publish(MqttService::TOPIC_STAT, "Calibrate first", 0, true);
    }
}

// Restore persisted states from EEPROM after boot.
// If power was lost while door was moving, enter error state for safety.
void StateMachine::init_states() {
    // EEPROM addresses for restoring saved states
    constexpr std::array addresses = {
        Eeprom::CALIB_ADDR, Eeprom::NEXT_DIR_ADDR, Eeprom::STEP_POS_ADDR,
        Eeprom::LOWEST_POS_ADDR, Eeprom::HIGHEST_POS_ADDR
    };
    // Pointers to variables to restore (first booleans, then 16-bit ints)
    const std::array bool_states = {&calibrated, &next_direction};
    const std::array int_states = {&motor_step_pos, &lowest_pos, &highest_pos};

    // Restore "door moving" flag first. If true, we treat it as unsafe shutdown.
    if (eeprom.validate_state(Eeprom::DOOR_MOV_ADDR)) {
        Eeprom::GenSt gst_door_mov;
        const int door_mov_num = init_st(gst_door_mov, Eeprom::DOOR_MOV_ADDR);
        door_moving = door_mov_num;

        if(!door_moving) {
            int index = 0;
            // Restore booleans (calibrated, next_direction) and then positions if calibrated.
            for (auto& addr : addresses) {
                if (index < 2) {
                    // 8-bit redundant states
                    if (eeprom.validate_state(addr)) {
                        Eeprom::GenSt gst;
                        const int num = init_st(gst, addr);
                        *bool_states[index] = num != 0;
                    }
                    else
                        std::cout << "State INVALID at addr: " << addr << "\n";
                }
                else if (calibrated) {
                    // 16-bit redundant states, only meaningful if calibrated is true
                    if (eeprom.validate_state16(addr)) {
                        Eeprom::GenSt16 gst;
                        *int_states[index - 2] = init_st16(gst, addr);
                    }
                    else
                        std::cout << "State16 INVALID at addr: " << addr << "\n";
                }
                index++;
            }
            std::cout << "Persistent states restored\n";
        }
        else {
            // If device rebooted while motor was moving, force error state for safety.
            std::cout << "Power loss during motor operation resulted in an error state\n";
            current_state = CurrentState::error;
        }
    }
    else
        std::cout << "Door moving state invalid.\n";
}

// Read a redundant 8-bit state from EEPROM and return the stored value.
int StateMachine::init_st(Eeprom::GenSt& gst, const uint16_t addr) const {
    eeprom.read_state(addr, &gst.state, &gst.not_state);
    return gst.state;
}

// Read a redundant 16-bit state from EEPROM and return the stored value.
int StateMachine::init_st16(Eeprom::GenSt16& gst, const uint16_t addr) const {
    eeprom.read_state16(addr, &gst.state, &gst.not_state);
    return gst.state;
}

// Convert current state enum to a string for logs and MQTT status publishing.
std::string StateMachine::get_st_string(const CurrentState st) {
    switch (st) {
        case CurrentState::idle: return "Idle state";
        case CurrentState::calib_open_door: return "Calibration open door state";
        case CurrentState::calib_close_door: return "Calibration close door state";
        case CurrentState::open_door: return "Open door state";
        case CurrentState::close_door: return "Close door state";
        case CurrentState::error: return "Error state";
        default: return "Idle state";
    }
}

// Non-blocking periodic helper.
// Returns true once per interval_ms when called repeatedly.
// Used to rate-limit stepping and blinking without blocking the main loop.
bool StateMachine::every_ms(const uint32_t interval_ms) {
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    // Initialize on first call after entering a new state.
    if (!last_ms_valid_) {
        last_ms_ = now;
        last_ms_valid_ = true;
    }
    // Interval elapsed -> trigger and reset timestamp.
    if (now - last_ms_ >= interval_ms) {
        last_ms_ = now;
        return true;
    }

    return false;
}

// Debug prints for calibration results.
void StateMachine::print_calib_info() const {
    std::cout << "Calibration completed.\n";
    std::cout << "Highest position: " << highest_pos << "\n";
    std::cout << "Lowest position: " << lowest_pos << "\n";
}

void StateMachine::to_error_st() {
    std::cout << "Recalibration required.\n";
    calibrated = false;
    eeprom.write_state(Eeprom::CALIB_ADDR, calibrated);
    next_state(CurrentState::error);
}