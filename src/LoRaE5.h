#ifndef LORA_H
#define LORA_H
#pragma once

#include <cstdint>
#include <cstddef>

#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

//static uart_inst_t* UART{uart1};

/*struct Config {
    uart_inst_t* uart_1 = UART;
    uint tx_pin = 4;   // GP4 -> LoRa RX
    uint rx_pin = 5;   // GP5 <- LoRa TX
    int baud_uart = 9600;
    const char* cmd_app_key = "AT+KEY=APPKEY, 93afdd36879296b1ca22e2923d60dddf";
};*/

class LoRaE5 {
public:
    //uart_inst_t* UART{uart1};

    enum class UartState : uint8_t {
        CheckConnection,
        CheckVersion,
        CheckDevEui,
        Stop
    };

    enum class JoinState : uint8_t {
        Mode,
        Key,
        ClassA,
        Port,
        Join,
        Stop
    };

    LoRaE5();

    void init_uart();

    // High-level operations
    bool check_comm();      // AT + VER + DevEui
    void join_network();    // MODE + KEY + CLASS + PORT + JOIN retries
    void send_msg(const char* msg_str);

    // Lower-level helpers (public if you want direct use)
    bool run_cmd(const char* cmd, const char* expected_token, int timeout_ms);

    void send_msg_hex_payload(int16_t temp_c_x100, uint16_t hum_x100, uint16_t bat_mv);

private:
    // AT tokens & timeouts
    static constexpr size_t LINE_LEN = 128;

    static constexpr const char* CMD_AT = "AT";
    static constexpr const char* CMD_AT_RETURN = "OK";

    static constexpr const char* CMD_VERSION = "AT+VER";
    static constexpr const char* CMD_VERSION_RETURN = "VER";

    static constexpr const char* CMD_DEV_EUI = "AT+ID=DevEui";
    static constexpr const char* CMD_DEV_EUI_RETURN = "DevEui";

    static constexpr const char* CMD_MODE = "AT+MODE=LWOTAA";
    static constexpr const char* CMD_MODE_RETURN = "LWOTAA";

    static constexpr const char* CMD_CLASS_A = "AT+CLASS=A";
    static constexpr const char* CMD_CLASS_A_RETURN = "+CLASS: A";

    static constexpr const char* CMD_PORT = "AT+PORT=8";
    static constexpr const char* CMD_PORT_RETURN = "+PORT: 8";

    static constexpr const char* CMD_JOIN = "AT+JOIN";
    static constexpr const char* CMD_JOIN_RETURN_TRUE = "+JOIN: Network joined";
    static constexpr const char* CMD_JOIN_RETURN_FALSE = "+JOIN: Join failed";

    static constexpr const char* CMD_MSG = "AT+MSG=";
    static constexpr const char* CMD_MSG_RETURN = "+MSG: Done";

    static constexpr int CMD_MS = 500;
    static constexpr int JOIN_MS = 20000;
    static constexpr int SEND_MSG_MS = 1250;

    // State machines
    void check_uart_sm_step(bool& continue_loop, bool& connected);
    void join_sm_step(bool& continue_loop);
    void join_with_retries();
    bool wait_join_result();

    void write_str(const char *string);
    bool read_line(char *buffer, int len, int timeout_ms);

    void pack_u16_le(uint8_t* dst, uint16_t v);
    void pack_i16_le(uint8_t* dst, int16_t v);
    void bytes_to_hex(const uint8_t* in, size_t len, char* out, size_t out_size);

    //Config cfg_;
    uart_inst_t* UART{uart1};
    uint tx_pin = 4;   // GP4 -> LoRa RX
    uint rx_pin = 5;   // GP5 <- LoRa TX
    int baud_uart = 9600;
    const char* cmd_app_key = "AT+KEY=APPKEY, 93afdd36879296b1ca22e2923d60dddf";

    UartState uart_state_ = UartState::CheckConnection;
    JoinState join_state_ = JoinState::Mode;
};


#endif