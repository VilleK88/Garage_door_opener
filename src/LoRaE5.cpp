#include "LoRaE5.h"
#include "Eeprom.h"

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <iostream>

LoRaE5::LoRaE5() {
    init_uart();
}


void LoRaE5::init_uart() {
    uart_init(UART, baud_uart);
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);

    uart_set_format(UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(UART, true);
}

bool LoRaE5::check_comm() {
    uart_state_ = UartState::CheckConnection;

    bool continue_loop = true;
    bool connected = false;

    while (continue_loop) {
        check_uart_sm_step(continue_loop, connected);
    }
    return connected;
}

void LoRaE5::join_network() {
    join_state_ = JoinState::Mode;

    bool continue_loop = true;
    while (continue_loop) {
        join_sm_step(continue_loop);
    }
}

bool LoRaE5::run_cmd(const char* cmd, const char* expected_token, int timeout_ms) {
    char line[LINE_LEN];

    for (int i = 0; i < 5; i++) {
        write_str(cmd);
        if (read_line(line, sizeof(line), timeout_ms)) {
            if (std::strstr(line, expected_token) != nullptr) {
                return true;
            }
        }
    }
    return false;
}

void LoRaE5::send_msg(const char* msg_str) {
    // LOG_ENTRY_SIZE comes from your project (used in C version).
    char message[Eeprom::LOG_ENTRY_SIZE];
    std::snprintf(message, sizeof(message), "%s\"%s\"", CMD_MSG, msg_str);

    char line[LINE_LEN];
    write_str(message);

    for (int i = 0; i < 5; i++) {
        if (read_line(line, sizeof(line), SEND_MSG_MS)) {
            if (std::strcmp(line, CMD_MSG_RETURN) == 0) {
                std::cout << "MSG send\n";
                return;
            }
        }
    }
}

void LoRaE5::check_uart_sm_step(bool& continue_loop, bool& connected) {
    switch (uart_state_) {
        case UartState::CheckConnection:
            if (run_cmd(CMD_AT, CMD_AT_RETURN, CMD_MS)) {
                std::printf("Connected to LoRa module\r\n");
                uart_state_ = UartState::CheckVersion;
            } else {
                std::printf("Module not responding\r\n");
                uart_state_ = UartState::Stop;
            }
            break;

        case UartState::CheckVersion:
            if (run_cmd(CMD_VERSION, CMD_VERSION_RETURN, CMD_MS)) {
                uart_state_ = UartState::CheckDevEui;
            } else {
                std::printf("Module stopped responding\r\n");
                uart_state_ = UartState::Stop;
            }
            break;

        case UartState::CheckDevEui:
            if (run_cmd(CMD_DEV_EUI, CMD_DEV_EUI_RETURN, CMD_MS)) {
                uart_state_ = UartState::CheckConnection;
                connected = true;
                continue_loop = false;
            } else {
                std::printf("Module stopped responding\r\n");
                uart_state_ = UartState::Stop;
            }
            break;

        case UartState::Stop:
            uart_state_ = UartState::CheckConnection;
            continue_loop = false;
            break;
    }
}

void LoRaE5::join_sm_step(bool& continue_loop) {
    switch (join_state_) {
        case JoinState::Mode:
            if (run_cmd(CMD_MODE, CMD_MODE_RETURN, CMD_MS)) {
                join_state_ = JoinState::Key;
            } else {
                join_state_ = JoinState::Stop;
            }
            break;

        case JoinState::Key:
            if (run_cmd(cmd_app_key, "+KEY: APPKEY", CMD_MS)) {
                join_state_ = JoinState::ClassA;
            } else {
                join_state_ = JoinState::Stop;
            }
            break;

        case JoinState::ClassA:
            if (run_cmd(CMD_CLASS_A, CMD_CLASS_A_RETURN, CMD_MS)) {
                join_state_ = JoinState::Port;
            } else {
                join_state_ = JoinState::Stop;
            }
            break;

        case JoinState::Port:
            if (run_cmd(CMD_PORT, CMD_PORT_RETURN, CMD_MS)) {
                join_state_ = JoinState::Join;
            } else {
                join_state_ = JoinState::Stop;
            }
            break;

        case JoinState::Join:
            join_with_retries();
            join_state_ = JoinState::Stop;
            break;

        case JoinState::Stop:
            join_state_ = JoinState::Mode;
            continue_loop = false;
            break;
    }
}

void LoRaE5::join_with_retries() {
    int count = 0;
    bool keep_trying = true;

    do {
        if (count < 5) {
            if (wait_join_result()) {
                keep_trying = false;
            }
        } else {
            keep_trying = false;
        }
        count++;
    } while (keep_trying);
}

bool LoRaE5::wait_join_result() {
    char line[LINE_LEN];

    write_str(CMD_JOIN);

    for (int i = 0; i < 5; i++) {
        if (read_line(line, sizeof(line), JOIN_MS)) {
            if (std::strcmp(line, CMD_JOIN_RETURN_TRUE) == 0) {
                std::printf("Joined to LoRaWAN network.\r\n");
                return true;
            }
            if (std::strcmp(line, CMD_JOIN_RETURN_FALSE) == 0) {
                std::printf("Connecting to LoRaWAN network failed.\r\n");
                return false;
            }
        }
    }
    return false;
}

void LoRaE5::write_str(const char *string) {
    while (*string) {
        uart_putc_raw(UART, *string++);
    }
    uart_putc_raw(UART, '\r');
    uart_putc_raw(UART, '\n');
}

bool LoRaE5::read_line(char *buffer, const int len, const int timeout_ms) {
    const uint32_t us = timeout_ms * 1000; // convert to microseconds
    // Wait for data to become available within timeout
    int i = 0;
    while (i <= len) {
        if (uart_is_readable_within_us(UART, us)) {
            const char c = uart_getc(UART);
            if (c != '\n') {
                if (c != '\r') // Ignore carriage return
                    buffer[i++] = c;
            }
            else break; // End of line
        }
        else return false; // No data received within timeout
    }
    buffer[i] = '\0'; // Null-terminate resulting string
    return true;
}

void LoRaE5::pack_u16_le(uint8_t* dst, uint16_t v) {
    dst[0] = static_cast<uint8_t>(v & 0xFF);
    dst[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
}

void LoRaE5::pack_i16_le(uint8_t* dst, const int16_t v) {
    pack_u16_le(dst, static_cast<uint16_t>(v));
}

void LoRaE5::bytes_to_hex(const uint8_t* in, const size_t len, char* out, const size_t out_size) {
    static constexpr char HEX[] = "0123456789ABCDEF";
    if (out_size < (len * 2 + 1)) return;

    for (size_t i = 0; i < len; i++) {
        out[i * 2 + 0] = HEX[(in[i] >> 4) & 0x0F];
        out[i * 2 + 1] = HEX[in[i] & 0x0F];
    }
    out[len * 2] = '\0';
}

void LoRaE5::send_msg_hex_payload(const int16_t temp_c_x100, const uint16_t hum_x100, const uint16_t bat_mv) {
    uint8_t payload[6];
    pack_i16_le(&payload[0], temp_c_x100);
    pack_u16_le(&payload[2], hum_x100);
    pack_u16_le(&payload[4], bat_mv);

    char hex_str[6 * 2 + 1]; // 12 + null
    bytes_to_hex(payload, sizeof(payload), hex_str, sizeof(hex_str));

    char cmd[64];
    std::snprintf(cmd, sizeof(cmd), "AT+MSG=\"%s\"", hex_str);

    char line[LINE_LEN];
    write_str(cmd);

    for (int i = 0; i < 5; i++) {
        if (read_line(line, sizeof(line), SEND_MSG_MS)) {
            if (std::strcmp(line, "+MSG: Done") == 0) {
                return;
            }
        }
    }
}