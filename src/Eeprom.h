#ifndef EEPROM_H
#define EEPROM_H
#pragma once

#include <cstdint>
#include <cstddef>
#include <array>

#include "hardware/i2c.h"
#include "pico/time.h"


class Eeprom final {
public:
    static constexpr int LOG_ENTRY_SIZE = 64;
    static constexpr int LOG_MAX_LEn = 61;
    static constexpr int MAG_LOGS = 32;
    static constexpr int WB_SLEEP_MS = 5;

    static constexpr uint16_t POS_ADDR = 32766;
    static constexpr uint16_t LOWEST_POS_ADDR = 32764;
    static constexpr uint16_t HIGHEST_POS_ADDR = 32762;
    static constexpr uint16_t CALIB_ADDR = 32760;
    static constexpr uint16_t NEXT_DIR_ADDR = 32758;
    static constexpr uint16_t DOOR_MOV_ADDR = 32756;

    struct GenSt {
        uint8_t state{};
        uint8_t not_state{};
    };
    struct GenSt16 {
        uint16_t state{};
        uint16_t not_state{};
    };

    Eeprom(i2c_inst_t* new_i2c, uint8_t new_address);

    void write_state(uint16_t addr, uint8_t state);
    void write_state_bytes(uint16_t addr, uint8_t state, uint8_t not_state);
    void read_state(uint16_t addr, uint8_t* state, uint8_t* not_state) const;
    bool validate_state(uint16_t addr) const;

    void write_byte(uint16_t addr, uint8_t value) const;
    uint8_t read_byte(uint16_t addr) const;

    static GenSt make_state(uint8_t state);

private:
    static bool state_is_valid(const GenSt& gst);

    i2c_inst_t* i2c;
    uint8_t address;
};


#endif