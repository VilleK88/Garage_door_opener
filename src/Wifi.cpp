#include "Wifi.h"
#include "config/wifi_config.h"
#include "../main.h"

#include <cstring>
#include <cstdio>

#include "pico/cyw43_arch.h"
#include "lwip/apps/Wifi.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"
#include "lwip/err.h"

#include "lwip/sockets.h"

extern queue_t events;

static constexpr const char* TOPIC_STATUS = "garage/door/status";

static constexpr const char* BROKER_IP_STR = MQTT_BROKER_IP;
static constexpr uint16_t BROKER_PORT  = 1883;

bool Wifi::connect_wifi() {
    const int init_rc = cyw43_arch_init_with_country(CYW43_COUNTRY_FINLAND);
    printf("cyw43_arch_init rc=%d\n", init_rc);

    if (init_rc) {
        std::cout << "cyw43 init failed\n";
    }
    else {
        std::cout << "cyw43 init completed\n";
        cyw43_arch_enable_sta_mode();

        const char* SSID = WIFI_SSID;
        const char* PASS = WIFI_PASS;

        if (cyw43_arch_wifi_connect_timeout_ms(SSID, PASS,
            CYW43_AUTH_WPA2_AES_PSK, 30000)) {
            std::cout << "Wifi connect failed\n";
            }
        else {
            connected = true;
            std::cout << "Wifi connection completed\n";
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

bool Wifi::init_udp(uint16_t local_port) {
    cyw43_arch_lwip_begin();

    pcb = udp_new();
    if (pcb) {
        err_t err = udp_bind(pcb, IP_ADDR_ANY, local_port);
        if (err == ERR_OK) {
            udp_recv(pcb, udp_receive_cb, this);
            cyw43_arch_lwip_end();
            std::cout << "UDP listening on port " << local_port << "\n";
            return true;
        }
        std::cout << "up_bind failed\n";
    }
    else
        std::cout << "udp_new failed\n";
    cyw43_arch_lwip_end();
    return false;
}

bool Wifi::send_msg(const char* dst_ip, const uint16_t dst_port, const char* msg) const {
    if (pcb && dst_ip && msg) {
        ip_addr_t dest_ip;
        if (ipaddr_aton(dst_ip, &dest_ip)) {
            size_t len = strlen(msg);

            pbuf* p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
            if (p) {
                memcpy(p->payload, msg, len);

                cyw43_arch_lwip_begin();
                const err_t err = udp_sendto(pcb, p, &dest_ip, dst_port);
                cyw43_arch_lwip_end();

                pbuf_free(p);

                if (err == ERR_OK) {
                    std::printf("UDP TX | %s:%u | %s\n", dst_ip, dst_port, msg);
                    sleep_ms(5000);
                    return true;
                }
                std::printf("udp_sendto failed err=%d\n", static_cast<int>(err));
            }
            else
                std::cout << "pbuf_alloc failed\n";
        }
        else
            std::cout << "Invalid IP\n";
    }
    return false;
}

void Wifi::udp_receive_cb(void* arg, udp_pcb*,
        pbuf* p, const ip_addr_t* addr, const u16_t port) {
    if (p) {
        char buffer[256]{};

        const size_t copy_len = p->len < sizeof(buffer) - 1 ? p->len : sizeof(buffer) - 1;
        memcpy(buffer, p->payload, copy_len);
        buffer[copy_len] = '\0';

        std::printf("UDP RX | %s:%u | %s\n", ipaddr_ntoa(addr), port, buffer);
        pbuf_free(p);
    }
}