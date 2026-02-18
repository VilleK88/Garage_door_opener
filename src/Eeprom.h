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

    static constexpr uint16_t CALIB_ADDR = 32766;
    static constexpr uint16_t DOOR_MOV_ADDR = 32764;
    static constexpr uint16_t NEXT_DIR_ADDR = 32762;
    static constexpr uint16_t POS_ADDR = 32758;
    static constexpr uint16_t LOWEST_POS_ADDR = 32754;
    static constexpr uint16_t HIGHEST_POS_ADDR = 32750;

    static constexpr std::array<uint16_t, 6> STATE_ADDRESSES {
        CALIB_ADDR,
        DOOR_MOV_ADDR,
        NEXT_DIR_ADDR,
        POS_ADDR,
        LOWEST_POS_ADDR,
        HIGHEST_POS_ADDR
    };

    struct GenSt {
        uint8_t state{};
        uint8_t not_state{};
    };
    struct GenSt16 {
        uint16_t state{};
        uint16_t not_state{};
    };

    Eeprom() = default;
    void init_eeprom();

    // --- Validated state (8-bit) ---
    void write_state(uint16_t addr, uint8_t state) const;
    void write_state_bytes(uint16_t addr, uint8_t state, uint8_t not_state) const;
    void read_state(uint16_t addr, uint8_t* state, uint8_t* not_state) const;
    [[nodiscard]] bool validate_state(uint16_t addr) const;

    // --- Validated state (16-bit) ---
    void write_state16(uint16_t addr, uint16_t state) const;
    void read_state16(uint16_t addr, uint16_t* state, uint16_t* not_state) const;
    [[nodiscard]] bool validate_state16(uint16_t addr) const;

    // --- Raw byte IO ---
    void write_byte(uint16_t addr, uint8_t value) const;
    [[nodiscard]] uint8_t read_byte(uint16_t addr) const;

    // Utility: build validated pairs
    [[nodiscard]] GenSt make_state(uint8_t state) const;
    [[nodiscard]] GenSt16 make_state16(uint16_t state) const;

private:
    [[nodiscard]] bool state_is_valid(const GenSt& gst) const;
    [[nodiscard]] bool state16_is_valid(const GenSt16& gst) const;

    void init_i2c() const;
    void init_memory() const;
    [[nodiscard]] bool valid_used_addresses() const;
    void erase_used_addresses() const;
    void init_addresses() const;

    static constexpr i2c_inst_t* I2C = i2c0;
    static constexpr uint32_t BAUD_RATE_I2C = 100000;
    static constexpr uint8_t EEPROM_ADDR = 0x50;

    static constexpr uint I2C_SDA = 16; // Serial Data Line
    static constexpr uint I2C_SCL = 17; // Serial Clock Line
    static constexpr std::array<uint, 2> i2cs { I2C_SDA, I2C_SCL };
};

#endif