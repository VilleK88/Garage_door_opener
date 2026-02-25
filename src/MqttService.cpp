#include "MqttService.h"
#include "../main.h" // event_t + EV_MQTT_CMD
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/util/queue.h"
#include "pico/cyw43_arch.h"
//#include "src/StateMachine.h"
#include "src/IPStack.h"

#include <iostream>
#include <vector>
#include <string>
#include <memory>

#include <cstdio>
#include <cstring>

#include "config/wifi_config.h"

extern queue_t events;

bool MqttService::connect(const char* broker_ip, const uint16_t port, const char* client_id) {
    if (!client) client = mqtt_client_new();
    if (client) {
        ip_addr_t addr{};
        if (ipaddr_aton(broker_ip, &addr)) {
            // Aseta vastaanottocallbackit ennen connectia
            mqtt_set_inpub_callback(client, &MqttService::on_incoming_publish, &MqttService::on_incoming_data, this);

            mqtt_connect_client_info_t ci{};
            ci.client_id = client_id;
            ci.client_user = nullptr;
            ci.client_pass = nullptr;
            ci.keep_alive = 30;

            // LWT: jos yhteys katkeaa yllättäen, broker julkaisee offline
            ci.will_topic = TOPIC_AVAIL;
            ci.will_msg = "offline";
            ci.will_qos = 0;
            ci.will_retain = 1;

            cyw43_arch_lwip_begin();
            const err_t err = mqtt_client_connect(client, &addr, port, &MqttService::on_mqtt_connection, this, &ci);
            cyw43_arch_lwip_end();

            if (err == ERR_OK)
                return true;
            printf("mqtt_client_connect failed: %d\n", static_cast<int>(err));
        }
        else
            printf("Invalid broker IP\n");
    }
    else
        printf("mqtt_client_new failed\n");
    return false;
}

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

void MqttService::on_mqtt_connection(mqtt_client_t* /*client*/, void* arg, const mqtt_connection_status_t status) {
    auto* self = static_cast<MqttService*>(arg);

    if (status == MQTT_CONNECT_ACCEPTED) {
        self->up = true;
        printf("MQTT connected\n");

        // Kerro että laite on online (retain)
        self->publish(TOPIC_AVAIL, "online", 0, true);

        // Tilaa komennot
        self->subscribe(TOPIC_CMD, 0);

        // Julkaise vaikka initial status (retain), jotta etäohjain näkee heti tilan
        self->publish(TOPIC_STAT, "BOOT", 0, true);
    } else {
        self->up = false;
        printf("MQTT connect failed, status=%d\n", static_cast<int>(status));
    }
}

void MqttService::on_mqtt_request(void* /*arg*/, const err_t result) {
    printf("MQTT request result=%d\n", static_cast<int>(result));
}

// Kutsutaan kun uusi publish alkaa (topic tiedossa, payloadia ei vielä)
void MqttService::on_incoming_publish(void* arg, const char* topic, u32_t /*tot_len*/) {
    auto* self = static_cast<MqttService*>(arg);
    std::snprintf(self->rx_topic, sizeof(self->rx_topic), "%s", topic ? topic : "");
    self->rx_len = 0;
    self->rx_buf[0] = '\0';
}

// Kutsutaan payload-palojen saapuessa; flags sisältää MQTT_DATA_FLAG_LAST viimeisessä palassa
void MqttService::on_incoming_data(void* arg, const u8_t* data, const u16_t len, const u8_t flags) {
    auto* self = static_cast<MqttService*>(arg);

    // Kokoa payload bufferiin (trunkkaa tarvittaessa)
    const size_t space = self->RX_MAX - 1 - self->rx_len;
    size_t copy_len = len < space ? len : space;

    if (copy_len > 0 && data) {
        std::memcpy(self->rx_buf + self->rx_len, data, copy_len);
        self->rx_len += copy_len;
        self->rx_buf[self->rx_len] = '\0';
    }

    if (flags & MQTT_DATA_FLAG_LAST) {
        // Nyt koko viesti koossa -> käsittele
        self->handle_command(self->rx_topic, self->rx_buf);

        // Reset
        self->rx_len = 0;
        self->rx_buf[0] = '\0';
    }
}
void MqttService::keep_connection_up() {
    static uint32_t last_try = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (!is_connected() && now - last_try > 5000) {
        last_try = now;
        connect(MY_IP_ADDR, CURRENT_PORT, "picoW-garage");
    }
}


bool MqttService::handle_commands(const event_t &event) {
    if (event.type == EV_MQTT_CMD) {
        std::cout << "Main got MQTT payload: " << event.payload << "\n";

        if (std::strcmp(event.payload, "STATUS") == 0) {
            publish("garage/door/status", "OK", 0, true);
            return false;
        }
        if (std::strcmp(event.payload, "TOGGLE") == 0) {
            // TODO: trigger_roller_relay_pulse(); tai sm.handle_door();
            publish("garage/door/status", "TOGGLING", 0, true);
            return true;
        }
        publish("garage/door/status", "UNKNOWN_CMD", 0, true);
        return false;
    }
}

void MqttService::handle_command(const char* topic, const char* payload) {
    if (!topic || !payload) return;
    if (std::strcmp(topic, TOPIC_CMD) != 0) return;

    // Push command to main loop via event queue
    event_t ev{};
    ev.type = EV_MQTT_CMD;
    ev.data = 1;

    std::strncpy(ev.payload, payload, sizeof(ev.payload) - 1);
    ev.payload[sizeof(ev.payload) - 1] = '\0';

    // Optional: drop if queue is full (you can print/debug if needed)
    queue_try_add(&events, &ev);
}