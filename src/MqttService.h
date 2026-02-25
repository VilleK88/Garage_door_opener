#ifndef MQTTSERVICE_H
#define MQTTSERVICE_H
#pragma once
#include <cstring>

#include "main.h"
#include "lwip/apps/mqtt.h"
#include "lwip/err.h"

class MqttService {
public:
    bool connect(const char* broker_ip, uint16_t port, const char* client_id);
    bool publish(const char* topic, const char* payload, int qos = 0, bool retain = false);
    bool subscribe(const char* topic, int qos = 0);

    [[nodiscard]] bool is_connected() const { return up; }

    void keep_connection_up();
    void handle_commands(const event_t &event);

    static constexpr auto TOPIC_CMD  = "garage/door/cmd";
    static constexpr auto TOPIC_STAT = "garage/door/status";
    static constexpr auto TOPIC_AVAIL = "garage/door/availability";

private:
    static void on_mqtt_connection(mqtt_client_t* client, void* arg, mqtt_connection_status_t status);
    static void on_mqtt_request(void* arg, err_t result);

    // Incoming publish callbacks
    static void on_incoming_publish(void* arg, const char* topic, u32_t tot_len);
    static void on_incoming_data(void* arg, const u8_t* data, u16_t len, u8_t flags);

    void handle_command(const char* topic, const char* payload);

    mqtt_client_t* client{nullptr};
    bool up{false};

    // Incoming message assembly (koska data voi tulla paloissa)
    static constexpr size_t RX_MAX = 256;
    char rx_topic[128]{};
    char rx_buf[RX_MAX]{};
    size_t rx_len{0};
};


#endif