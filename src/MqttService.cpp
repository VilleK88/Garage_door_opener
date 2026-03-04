#include "MqttService.h"
#include "../main.h"
#include "utils/events.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/cyw43_arch.h"
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <cstdio>
#include <cstring>
#include "config/wifi_config.h"

// Create/connect lwIP MQTT client, configure callbacks, set keep-alive and LWT.
bool MqttService::connect(const char* broker_ip, const uint16_t port, const char* client_id) {
    // Create client object on first use
    if (!client) client = mqtt_client_new();

    if (client) {
        ip_addr_t addr{};
        // Convert IP string to ip_addr_t
        if (ipaddr_aton(broker_ip, &addr)) {
            // Set receive callbacks BEFORE connecting so incoming messages are handled immediately.
            mqtt_set_inpub_callback(client, &MqttService::on_incoming_publish, &MqttService::on_incoming_data, this);

            // Connection parameters
            mqtt_connect_client_info_t ci{};
            ci.client_id = client_id;
            ci.client_user = nullptr;
            ci.client_pass = nullptr;
            ci.keep_alive = 30;

            // Last Will and Testament (LWT):
            // If the device disconnects unexpectedly, broker publishes "offline" retained.
            ci.will_topic = TOPIC_AVAIL;
            ci.will_msg = "offline";
            ci.will_qos = 0;
            ci.will_retain = 1;

            // lwIP calls must be protected with cyw43_arch_lwip_begin/end on Pico W.
            cyw43_arch_lwip_begin();
            const err_t err = mqtt_client_connect(client, &addr, port, &MqttService::on_mqtt_connection, this, &ci);
            cyw43_arch_lwip_end();

            if (err == ERR_OK)
                return true;
            printf("mqtt client connect failed: %d\n", static_cast<int>(err));
        }
        else
            printf("Invalid broker IP\n");
    }
    else
        printf("mqtt_client_new failed\n");
    return false;
}

// Publish message to broker (requires active connection).
bool MqttService::publish(const char* topic, const char* payload, int qos, bool retain) {
    if (client && up) {
        cyw43_arch_lwip_begin();
        const err_t err = mqtt_publish(client, topic, payload, static_cast<u16_t>(std::strlen(payload)),
                                 static_cast<u8_t>(qos), retain ? 1 : 0, &MqttService::on_mqtt_request, this);
        cyw43_arch_lwip_end();

        if (err == ERR_OK)
            return true;
        printf("mqtt_publish failed: %d\n", static_cast<int>(err));
    }
    return false;
}

// Subscribe to a topic (requires active connection).
bool MqttService::subscribe(const char* topic, const int qos) {
    if (client && up) {
        cyw43_arch_lwip_begin();
        const err_t err = mqtt_subscribe(client, topic, static_cast<u8_t>(qos), &MqttService::on_mqtt_request, this);
        cyw43_arch_lwip_end();

        if (err == ERR_OK)
            return true;
        printf("mqtt_subscribe failed: %d\n", static_cast<int>(err));
    }
    return false;
}

// Connection callback from lwIP MQTT.
// Sets state, announces availability, subscribes to command topic, publishes initial status.
void MqttService::on_mqtt_connection(mqtt_client_t* /*client*/, void* arg, const mqtt_connection_status_t status) {
    auto* self = static_cast<MqttService*>(arg);

    if (status == MQTT_CONNECT_ACCEPTED) {
        self->up = true;
        printf("MQTT connected\n");

        // Publish retained "online" so remote controllers can see device availability.
        self->publish(TOPIC_AVAIL, "online", 0, true);

        // Subscribe to the command topic.
        self->subscribe(TOPIC_CMD, 0);

        if (!self->all_ready_conn) {
            self->publish(TOPIC_STAT, "Boot", 0, true);
            self->all_ready_conn = true;
        }
        else
            self->publish(TOPIC_STAT, "Connection re-established", 0, true);
    } else {
        self->up = false;
        printf("MQTT connect failed, status=%d\n", static_cast<int>(status));
    }
}

// Called when lwIP finishes a publish/subscribe request.
void MqttService::on_mqtt_request(void* /*arg*/, const err_t result) {
    printf("MQTT request result=%d\n", static_cast<int>(result));
}

// Called when a new inbound PUBLISH begins.
// Stores topic and resets the payload assembly buffer.
void MqttService::on_incoming_publish(void* arg, const char* topic, u32_t /*tot_len*/) {
    auto* self = static_cast<MqttService*>(arg);

    // Copy topic safely into rx_topic
    std::snprintf(self->rx_topic, sizeof(self->rx_topic), "%s", topic ? topic : "");

    // Reset payload buffer state
    self->reset_buffer();
}

// Called for each incoming payload chunk.
// Payload may arrive in multiple fragments; MQTT_DATA_FLAG_LAST marks the final fragment.
void MqttService::on_incoming_data(void* arg, const u8_t* data, const u16_t len, const u8_t flags) {
    auto* self = static_cast<MqttService*>(arg);

    // Remaining space (leave room for '\0')
    const size_t space = self->RX_MAX - 1 - self->rx_len;
    size_t copy_len = len < space ? len : space;

    // Append chunk into buffer (truncate if too large)
    if (copy_len > 0 && data) {
        std::memcpy(self->rx_buf + self->rx_len, data, copy_len);
        self->rx_len += copy_len;
        self->rx_buf[self->rx_len] = '\0';
    }

    // Final chunk received -> process complete message
    if (flags & MQTT_DATA_FLAG_LAST) {
        // Nyt koko viesti koossa -> käsittele
        self->handle_command(self->rx_topic, self->rx_buf);

        // Reset buffer for next message
        self->reset_buffer();
    }
}

// Periodic reconnect logic: if disconnected, try to reconnect every 5 seconds.
void MqttService::keep_connection_up() {
    static uint32_t last_try = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (!is_connected() && now - last_try > 5000) {
        last_try = now;
        if (connect(IP_ADDR, CURRENT_PORT, "picoW-garage")) {
            std::cout << "Connection restored\n";
        }
    }
}

// Handle MQTT command events in the main loop.
// Interprets payload and publishes a status update.
// Returns true if caller should trigger an action (toggle/calibrate).
bool MqttService::handle_commands(const event_t &event) {
    if (event.type == EV_MQTT_CMD) {
        std::cout << "Main got MQTT payload: " << event.payload << "\n";

        if (std::strcmp(event.payload, "STATUS") == 0) {
            publish("garage/door/status", "OK", 0, true);
            current_cmd = STATUS;
            return false; // query only
        }
        if (std::strcmp(event.payload, "TOGGLE") == 0) {
            // Intended: trigger door toggle in main/state machine.
            publish("garage/door/status", "TOGGLING", 0, true);
            current_cmd = TOGGLE;
            return true;
        }
        if (std::strcmp(event.payload, "CALIBRATE") == 0) {
            publish("garage/door/status", "CALIBRATION", 0, true);
            current_cmd = CALIBRATE;
            return true;
        }
        publish("garage/door/status", "UNKNOWN_CMD", 0, true);
        current_cmd = UNKNOWN_CMD;
        return false;
    }
    return false;
}

// Process a complete inbound MQTT message.
// If it is on the command topic, forward it to the main loop by pushing EV_MQTT_CMD to the queue.
void MqttService::handle_command(const char* topic, const char* payload) {
    if (topic && payload) {
        // Only accept commands on the expected topic
        if (std::strcmp(topic, TOPIC_CMD) == 0) {

            // Create event to be handled in main loop (not in lwIP callback context)
            event_t ev{};
            ev.type = EV_MQTT_CMD;
            ev.data = 1;

            // Copy payload safely into event buffer
            std::strncpy(ev.payload, payload, sizeof(ev.payload) - 1);
            ev.payload[sizeof(ev.payload) - 1] = '\0';

            // Push to global event queue; if full, event is dropped.
            queue_try_add(&events, &ev);
        }
    }
}

void MqttService::reset_buffer() {
    rx_len = 0;
    rx_buf[0] = '\0';
}