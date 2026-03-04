#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H
#pragma once
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

// Simple Wi-Fi helper class for Pico W.
// Responsible for:
// 1) Initializing the CYW43 Wi-Fi chip
// 2) Connecting the device to an access point
class Wifi final {
public:
    Wifi() = default;

    // Initializes the CYW43 Wi-Fi driver.
    // Must be called before any Wi-Fi operations.
    // Returns true if initialization succeeded.
    bool init();

    // Connects the Pico W to the configured Wi-Fi network.
    // Uses credentials defined in wifi_config.h.
    // Returns true when the device is successfully connected.
    bool connect_wifi();

private:
    bool connected{false}; // Indicates whether Wi-Fi connection is active
    int CONN_TIMEOUT_MS{30000}; // Connection timeout in milliseconds
};

#endif