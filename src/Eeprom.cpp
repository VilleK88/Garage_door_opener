#include "Eeprom.h"

#include <cstring>
#include <cstdio>

Eeprom::Eeprom(i2c_inst_t* new_i2c, const uint8_t new_address)
    : i2c(new_i2c), address(new_address) {}

void Eeprom::write_state(uint16_t addr, uint8_t state) {
    const auto gst = make_state(state);
    write_state_bytes(addr, gst.state, gst.not_state);
}

void Eeprom::write_state_bytes(uint16_t addr, uint8_t state, uint8_t not_state) {
    const uint8_t buffer[4] = {
        static_cast<uint8_t>(addr >> 8 & 0xFF),
        static_cast<uint8_t>(addr & 0xFF),
        state, not_state
    };
    i2c_write_blocking(i2c, addr, buffer, 4, false);
    sleep_ms(WB_SLEEP_MS);
}

void Eeprom::read_state(uint16_t addr, uint8_t* state, uint8_t* not_state) const {
    const uint8_t addr_part[2] = {
        static_cast<uint8_t>(addr >> 8 & 0xFF),
        static_cast<uint8_t>(addr & 0xFF)
    };

    uint8_t data[2]{};
    i2c_write_blocking(i2c, addr, addr_part, 2, true);
    i2c_read_blocking(i2c, addr, data, 2, false);

    *state = data[0];
    *not_state = data[1];
}

bool Eeprom::validate_state(const uint16_t addr) const {
    GenSt gst;
    read_state(addr, &gst.state, &gst.not_state);
    return state_is_valid(gst);
}

void Eeprom::write_byte(const uint16_t addr, const uint8_t value) const {
    const uint8_t buffer[3] = {
        static_cast<uint8_t>((addr >> 8) & 0xFF),
        static_cast<uint8_t>(addr & 0xFF), value
    };
    i2c_write_blocking(i2c, addr, buffer, 3, false);
    sleep_ms(WB_SLEEP_MS);
}

uint8_t Eeprom::read_byte(const uint16_t addr) const {
    const uint8_t addr_part[2] = {
        static_cast<uint8_t>(addr >> 8 & 0xFF),
        static_cast<uint8_t>(addr & 0xFF)
    };
    uint8_t data{};
    i2c_write_blocking(i2c, addr, addr_part, 2, true);
    i2c_read_blocking(i2c, addr, &data, 1, false);
    return data;
}

Eeprom::GenSt Eeprom::make_state(uint8_t state) {
    return GenSt{ state, static_cast<uint8_t>(~state) };
}

bool Eeprom::state_is_valid(const GenSt& gst) {
    return gst.state == static_cast<uint8_t>(~gst.not_state);
}