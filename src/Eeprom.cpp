#include "Eeprom.h"

#include <cstring>
#include <cstdio>

#include "hardware/gpio.h"

void Eeprom::init_eeprom() {
    init_i2c();
    init_memory();
}

void Eeprom::write_state(const uint16_t addr, const uint8_t state) const {
    const auto gst = make_state(state);
    write_state_bytes(addr, gst.state, gst.not_state);
}

void Eeprom::write_state_bytes(const uint16_t addr, const uint8_t state, const uint8_t not_state) const {
    const uint8_t buffer[4] = {
        static_cast<uint8_t>(addr >> 8 & 0xFF),
        static_cast<uint8_t>(addr & 0xFF),
        state, not_state
    };
    i2c_write_blocking(I2C, EEPROM_ADDR, buffer, 4, false);
    sleep_ms(WB_SLEEP_MS);
}

void Eeprom::read_state(const uint16_t addr, uint8_t* state, uint8_t* not_state) const {
    const uint8_t addr_part[2] = {
        static_cast<uint8_t>(addr >> 8 & 0xFF),
        static_cast<uint8_t>(addr & 0xFF)
    };

    uint8_t data[2]{};
    i2c_write_blocking(I2C, EEPROM_ADDR, addr_part, 2, true);
    i2c_read_blocking(I2C, EEPROM_ADDR, data, 2, false);

    *state = data[0];
    *not_state = data[1];
}

bool Eeprom::validate_state(const uint16_t addr) const {
    GenSt gst;
    read_state(addr, &gst.state, &gst.not_state);
    return state_is_valid(gst);
}

void Eeprom::write_state16(const uint16_t addr, const uint16_t state) const {
    const auto gst = make_state16(state);

    const uint8_t buffer[6] = {
        static_cast<uint8_t>(addr >> 8 & 0xFF),
        static_cast<uint8_t>(addr & 0xFF),
        static_cast<uint8_t>(gst.state >> 8 & 0xFF),
        static_cast<uint8_t>(gst.state & 0xFF),
        static_cast<uint8_t>(gst.not_state >> 8 & 0xFF),
        static_cast<uint8_t>(gst.not_state & 0xFF)
    };

    i2c_write_blocking(I2C, EEPROM_ADDR, buffer, sizeof(buffer), false);
    sleep_ms(WB_SLEEP_MS);
}

void Eeprom::read_state16(const uint16_t addr, uint16_t* state, uint16_t* not_state) const {
    const uint8_t addr_part[2] = {
        static_cast<uint8_t>(addr >> 8 & 0xFF),
        static_cast<uint8_t>(addr & 0xFF)
    };

    uint8_t data[4]{};
    i2c_write_blocking(I2C, EEPROM_ADDR, addr_part, 2, true);
    i2c_read_blocking(I2C, EEPROM_ADDR, data, 4, false);

    *state = static_cast<uint16_t>(data[0] << 8 | data[1]);
    *not_state = static_cast<uint16_t>(data[2] << 8 | data[3]);
}

bool Eeprom::validate_state16(const uint16_t addr) const {
    GenSt16 gst;
    read_state16(addr, &gst.state, &gst.not_state);
    return state16_is_valid(gst);
}

void Eeprom::write_byte(const uint16_t addr, const uint8_t value) const {
    const uint8_t buffer[3] = {
        static_cast<uint8_t>((addr >> 8) & 0xFF),
        static_cast<uint8_t>(addr & 0xFF), value
    };
    i2c_write_blocking(I2C, EEPROM_ADDR, buffer, 3, false);
    sleep_ms(WB_SLEEP_MS);
}

uint8_t Eeprom::read_byte(const uint16_t addr) const {
    const uint8_t addr_part[2] = {
        static_cast<uint8_t>(addr >> 8 & 0xFF),
        static_cast<uint8_t>(addr & 0xFF)
    };
    uint8_t data{};
    i2c_write_blocking(I2C, EEPROM_ADDR, addr_part, 2, true);
    i2c_read_blocking(I2C, EEPROM_ADDR, &data, 1, false);
    return data;
}

Eeprom::GenSt Eeprom::make_state(const uint8_t state) const {
    return GenSt{ state, static_cast<uint8_t>(~state) };
}

Eeprom::GenSt16 Eeprom::make_state16(const uint16_t state) const {
    return GenSt16{ state, static_cast<uint16_t>(~state) };
}

bool Eeprom::state_is_valid(const GenSt& gst) const {
    return gst.state == static_cast<uint8_t>(~gst.not_state);
}

bool Eeprom::state16_is_valid(const GenSt16& gst) const {
    return gst.state == static_cast<uint16_t>(~gst.not_state);
}

void Eeprom::init_i2c() const {
    i2c_init(I2C, BAUD_RATE_I2C);
    for (const auto pin : i2cs) {
        gpio_set_function(pin, GPIO_FUNC_I2C);
        gpio_pull_up(pin);
    }
}

void Eeprom::init_memory() const {
    if (!valid_used_addresses()) {
        erase_used_addresses();
        init_addresses();
    }
}

bool Eeprom::valid_used_addresses() const {
    int index = 0;
    for (const auto addr : STATE_ADDRESSES) {
        if (index < 3) {
            if (!validate_state16(addr))
                return false;
        }
        else {
            if (!validate_state(addr))
                return false;
        }
        index++;
    }
    return true;
}

void Eeprom::erase_used_addresses() const {
    uint16_t min_addr = STATE_ADDRESSES[0];
    uint16_t max_addr = STATE_ADDRESSES[0];

    for (const auto addr : STATE_ADDRESSES) {
        if (addr < min_addr)
            min_addr = addr;
        if (addr > max_addr)
            max_addr = addr;
    }

    for (uint16_t addr = min_addr; addr <= static_cast<uint16_t>(max_addr + 1); addr++) {
        write_byte(addr, 0xFF);
    }
}

void Eeprom::init_addresses() const {
    int index = 0;
    for (auto addr : STATE_ADDRESSES) {
        if (index < 3)
            write_state(addr, 0);
        else
            write_state16(addr, 0);
        index++;
    }
}