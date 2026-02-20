#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <string>
#include <cstdint>

#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
/*extern "C" {
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
}*/


class MqttClient final {
public:
    MqttClient() = default;

    void connect();
    void subscribe();
    void publish(const char* json_payload);
    void poll();
    std::string make_status_json();
    void handle_cmd(const std::string& payload);

private:
    bool connected{false};
    mqtt_client_t *client{nullptr};

    char rx_topic[64]{};
    char rx_payload[128]{};
    uint32_t rx_len{0};

    static void s_connection_cb(mqtt_client_t* new_client, void* arg, int status);
    static void s_incoming_publish_cb(void* arg, const char* topic, uint32_t tot_len);
    static void s_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);

    void on_full_msg(const char* topic, const char* payload);
};

#endif