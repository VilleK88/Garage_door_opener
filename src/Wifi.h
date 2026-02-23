#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <string>
#include <cstdint>

#include "pico/cyw43_arch.h"
#include "lwip/apps/Wifi.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

class Wifi final {
public:
    Wifi() = default;

    bool connect_wifi();
    bool init_udp(uint16_t local_port);
    bool send_msg(const char* dst_ip, uint16_t dst_port, const char* msg) const;

private:
    bool connected{false};
    static void udp_receive_cb(void* arg, udp_pcb* pcb,
        pbuf* p, const ip_addr_t* addr, u16_t port);
    udp_pcb* pcb{nullptr};
};

#endif