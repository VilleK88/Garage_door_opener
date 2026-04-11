#include "Wifi.h"
#include "config/wifi_config.h"
#include <cstring>
#include <cstdio>
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"
#include "pico/util/queue.h"
#include "pico/stdlib.h"
#include "src/StateMachine.h"
#include "src/RotaryEncoder.h"

// Initialize CYW43 Wi-Fi chip and networking stack
bool Wifi::init() {

    // Initialize Wi-Fi chip with selected country regulatory domain
    const int init_rc = cyw43_arch_init_with_country(CYW43_COUNTRY_FINLAND);
    printf("cyw43_arch_init rc=%d\n", init_rc);

    // Initialization successful
    if (!init_rc) {
        std::cout << "cyw43 init completed\n";
        return true;
    }
    // Initialization failed
    std::cout << "cyw43 init failed\n";
    return false;
}

// Connect Pico W to Wi-Fi access point
bool Wifi::connect_wifi() {

    // Enable station mode (client mode)
    cyw43_arch_enable_sta_mode();

    // Load Wi-Fi credentials from configuration file
    auto SSID = WIFI_SSID;
    auto PASS = WIFI_PASS;

    // Attempt connection with timeout
    if (cyw43_arch_wifi_connect_timeout_ms(SSID, PASS,
        CYW43_AUTH_WPA2_AES_PSK, CONN_TIMEOUT_MS)) {
        // Connection attempt failed
        std::cout << "Wi-Fi connect failed\n";
        return connected;
        }

    // Connection established
    connected = true;
    std::cout << "Wi-Fi connected\n";
    return connected;
}