#include "Wifi.h"
#include "config/wifi_config.h"
#include "../main.h"

#include <cstring>
#include <cstdio>

#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"

#include "lwip/sockets.h"

extern queue_t events;

static constexpr const char* TOPIC_STATUS = "garage/door/status";

bool Wifi::connect_wifi() {
    const int init_rc = cyw43_arch_init_with_country(CYW43_COUNTRY_FINLAND);
    printf("cyw43_arch_init rc=%d\n", init_rc);

    if (init_rc) {
        std::cout << "cyw43 init failed\n";
    }
    else {
        std::cout << "cyw43 init completed\n";
        cyw43_arch_enable_sta_mode();

        auto SSID = WIFI_SSID;
        auto PASS = WIFI_PASS;

        if (cyw43_arch_wifi_connect_timeout_ms(SSID, PASS,
            CYW43_AUTH_WPA2_AES_PSK, CONN_TIMEOUT_MS)) {
            std::cout << "Wifi connect failed\n";
            }
        else {
            connected = true;
            std::cout << "Wifi connected\n";
            while (!netif_default || !netif_is_up(netif_default) ||
                ip4_addr_isany_val(*netif_ip4_addr(netif_default))) {
                cyw43_arch_poll();
                sleep_ms(100);
                }
            printf("Got IP: %s\n", ipaddr_ntoa(netif_ip_addr4(netif_default)));
            return connected;
        }
    }
    return connected;
}

/*bool Wifi::tcp_connect(const char* server_ip, uint16_t server_port) {
    if (connected && server_ip) {
        ip_addr_t dest_ip{};
        if (ipaddr_aton(server_ip, &dest_ip)) {
            cyw43_arch_lwip_begin();

            pcb = tcp_new_ip_type(IP_GET_TYPE(&dest_ip));
            if (pcb) {
                tcp_arg(pcb, this);
                tcp_err(pcb, &on_tcp_err);
                tcp_recv(pcb, &on_tcp_recv);
                tcp_sent(pcb, &on_tcp_sent);

                err_t err = ::tcp_connect(pcb, &dest_ip, server_port, &on_tcp_connected);

                cyw43_arch_lwip_end();

                if (err == ERR_OK) {
                    std::printf("TCP connecting to %s:%u...\n", server_ip, server_port);
                    return true;
                }
                std::printf("tcp_connect start failed err=%d\n", (int)err);
            }
            else
                std::printf("tcp_new failed\n");
        }
        else
            std::printf("Invalid server IP: %s\n", server_ip);
    }
    cyw43_arch_lwip_end();
    close_connection();
    return false;
}*/