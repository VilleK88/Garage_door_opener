#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H
#pragma once
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"


class Wifi final {
public:
    Wifi() = default;
    bool connect_wifi();

private:
    bool connected{false}; // Wi-Fi connected
    //tcp_pcb* pcb{nullptr}; // Active TCP PCB
    //bool tcp_up{false}; // TCP connected
    int CONN_TIMEOUT_MS{30000};
};

#endif