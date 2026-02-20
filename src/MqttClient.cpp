#include "MqttClient.h"
#include "config/wifi_config.h"

#include <cstring>
#include <cstdio>

//#include "pico/stdlib.h"
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"

#include "../main.h"
extern queue_t events;

static constexpr const char* TOPIC_STATUS = "garage/door/status";

static constexpr const char* BROKER_IP_STR = MQTT_BROKER_IP;
static constexpr uint16_t BROKER_PORT  = 1883;

void MqttClient::connect() {
    // Create client
    client = mqtt_client_new();
    if (client) {
        // Set incoming callbacks
        mqtt_set_inpub_callback(
            client,
            &MqttClient::s_incoming_publish_cb,
            reinterpret_cast<mqtt_incoming_data_cb_t>(&MqttClient::s_incoming_data_cb),
            this
            );

        // Broker IP
        ip_addr_t broker_ip{};
        ip4addr_aton(BROKER_IP_STR, ip_2_ip4(&broker_ip));

        // Connect options
        mqtt_connect_client_info_t ci{};
        ci.client_id = "pico-garage";
        ci.keep_alive = 20;

        // Connect
        const err_t err = mqtt_client_connect(
            client,
            &broker_ip,
            BROKER_PORT,
            reinterpret_cast<mqtt_connection_cb_t>(&MqttClient::s_connection_cb),
            this,
            &ci
            );

        if (err != ERR_OK) {
            std::printf("MQTT: connect start failed err=%d\n", static_cast<int>(err));
        }

    }
    else {
        std::printf("MQTT: mqtt_client_new failed\n");
    }
}

void MqttClient::subscribe() {

}

void MqttClient::publish(const char* json_payload) {
    if (connected && client) {
        const size_t len = std::strlen(json_payload);
        const err_t err = mqtt_publish(
            client,
            TOPIC_STATUS,
            json_payload,
            len,
            1, // QoS
            1, // Retained
            nullptr,
            nullptr
            );

        if (err != ERR_OK) {
            std::printf("MQTT: publish failed err=%d\n", static_cast<int>(err));
        }

    }
}

void MqttClient::poll() {

}

std::string MqttClient::make_status_json() {
    /*return "{"
        "\"door\n:\n" + door_to_string(st.door) + "\","
        "\"error\":\"" + error_to_string(st.error) + "\","
        "\"calibration\":\"" + calib_to_string(st.calib) + "\""
    "}";*/
}

void MqttClient::handle_cmd(const std::string& payload) {

}

void MqttClient::s_connection_cb(mqtt_client_t* new_client, void* arg, int status) {
    auto* self = static_cast<MqttClient*>(arg);

    self->connected = (status == MQTT_CONNECT_ACCEPTED);
    std::printf("MQTT: connection status =%d\n", status);

    if (self->connected) {
        const err_t err = mqtt_subscribe(
            new_client,
            TOPIC_STATUS,
            1,
            nullptr,
            nullptr
            );

        if (err == ERR_OK) {
            std::printf("MQTT: subscribed to %s\n", TOPIC_STATUS);
            self->publish("{\"door\":\"OPEN\",\"error\":\"NORMAL\",\"calibrated\":true}");
        }
        else
            std::printf("MQTT: subscribe failed err =%d\n", static_cast<int>(err));
    }
}

void MqttClient::s_incoming_publish_cb(void* arg, const char* topic, uint32_t tot_len) {
    auto* self = static_cast<MqttClient*>(arg);

    self->rx_len = 0;
    std::strncpy(self->rx_topic, topic, sizeof(self->rx_topic) - 1);
    self->rx_topic[sizeof(self->rx_topic) - 1] = '\0';
    self->rx_payload[0] = '\0';
}

void MqttClient::s_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags) {
    auto* self = static_cast<MqttClient*>(arg);

    const size_t space = sizeof(self->rx_payload) - 1 - self->rx_len;
    const size_t copy_len = len < space ? len : space;

    std::memcpy(self->rx_payload + self->rx_len, data, copy_len);
    self->rx_len += copy_len;
    self->rx_payload[self->rx_len] = '\0';

    if (flags & MQTT_DATA_FLAG_LAST) {
        self->on_full_msg(self->rx_topic, self->rx_payload);
    }
}

void MqttClient::on_full_msg(const char* topic, const char* payload) {
    event_t ev{};
    ev.type = EV_MQTT_CMD;
    ev.data = 1;

    std::strncpy(ev.payload, payload, sizeof(ev.payload) - 1);
    ev.payload[sizeof(ev.payload) - 1] = '\0';

    queue_try_add(&events, &ev);

    std::printf("MQTT RX | %s | %s\n", topic, payload);
}