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

bool Wifi::init() {
    const int init_rc = cyw43_arch_init_with_country(CYW43_COUNTRY_FINLAND);
    printf("cyw43_arch_init rc=%d\n", init_rc);
    if (!init_rc) {
        std::cout << "cyw43 init completed\n";
        return true;
    }
    std::cout << "cyw43 init failed\n";
    return false;
}

bool Wifi::connect_wifi() {
    cyw43_arch_enable_sta_mode();

    auto SSID = WIFI_SSID;
    auto PASS = WIFI_PASS;

    if (cyw43_arch_wifi_connect_timeout_ms(SSID, PASS,
        CYW43_AUTH_WPA2_AES_PSK, CONN_TIMEOUT_MS)) {
        std::cout << "Wi-Fi connect failed\n";
        return connected;
        }
    connected = true;
    std::cout << "Wi-Fi connected\n";
    while (!netif_default || !netif_is_up(netif_default) ||
        ip4_addr_isany_val(*netif_ip4_addr(netif_default))) {
        cyw43_arch_poll();
        sleep_ms(100);
        }
    //printf("Got IP: %s\n", ipaddr_ntoa(netif_ip_addr4(netif_default)));
    return connected;
}