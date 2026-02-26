#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H
#pragma once
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

class Wifi final {
public:
    Wifi() = default;
    bool init();
    bool connect_wifi();

private:
    bool connected{false}; // Wi-Fi connected
    int CONN_TIMEOUT_MS{30000};
};

#endif