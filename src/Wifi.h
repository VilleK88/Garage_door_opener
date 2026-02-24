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
    //bool init_udp(uint16_t local_port);
    //bool send_msg(const char* dst_ip, uint16_t dst_port, const char* msg) const;

    // Connect to a TCP server (PC) and set callbacks
    bool tcp_connect(const char* server_ip, uint16_t server_port);

    // Send a text message over the active TCP connection
    bool tcp_send(const char* msg);

private:
    bool connected{false}; // Wi-Fi connected
    //static void udp_receive_cb(void* arg, udp_pcb* pcb, pbuf* p, const ip_addr_t* addr, u16_t port);
    //udp_pcb* pcb{nullptr};
    tcp_pcb* pcb{nullptr}; // Active TCP PCB
    bool tcp_up{false}; // TCP connected

    int CONN_TIMEOUT_MS{30000};

    // lwIP TCP callbacks
    static err_t on_tcp_connected(void* arg, tcp_pcb* tpcb, err_t err);
    static err_t on_tcp_recv(void* arg, tcp_pcb* tpcb, pbuf* p, err_t err);
    static err_t on_tcp_sent(void* arg, tcp_pcb* tpcb, u16_t len);
    static void  on_tcp_err(void* arg, err_t err);

    void close_connection();
};

#endif