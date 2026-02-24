#include "MqttService.h"

#include <cstdio>
#include <cstring>

#include "pico/cyw43_arch.h"
#include "pico/util/queue.h"
extern queue_t events;

#include "../main.h" // event_t + EV_MQTT_CMD

static constexpr const char* TOPIC_CMD  = "garage/door/cmd";
static constexpr const char* TOPIC_STAT = "garage/door/status";
static constexpr const char* TOPIC_AVAIL = "garage/door/availability";

bool MqttService::connect(const char* broker_ip, uint16_t port, const char* client_id) {
    if (!client) client = mqtt_client_new();
    if (!client) {
        printf("mqtt_client_new failed\n");
        return false;
    }

    ip_addr_t addr{};
    if (!ipaddr_aton(broker_ip, &addr)) {
        printf("Invalid broker IP\n");
        return false;
    }

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
    err_t err = mqtt_client_connect(client, &addr, port, &MqttService::on_mqtt_connection, this, &ci);
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        printf("mqtt_client_connect failed: %d\n", (int)err);
        return false;
    }
    return true;
}

bool MqttService::publish(const char* topic, const char* payload, int qos, bool retain) {
    if (!client || !up) return false;

    cyw43_arch_lwip_begin();
    err_t err = mqtt_publish(client, topic, payload, (u16_t)std::strlen(payload),
                             (u8_t)qos, retain ? 1 : 0, &MqttService::on_mqtt_request, this);
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        printf("mqtt_publish failed: %d\n", (int)err);
        return false;
    }
    return true;
}

bool MqttService::subscribe(const char* topic, int qos) {
    if (!client || !up) return false;

    cyw43_arch_lwip_begin();
    err_t err = mqtt_subscribe(client, topic, (u8_t)qos, &MqttService::on_mqtt_request, this);
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        printf("mqtt_subscribe failed: %d\n", (int)err);
        return false;
    }
    return true;
}

void MqttService::on_mqtt_connection(mqtt_client_t* /*client*/, void* arg, mqtt_connection_status_t status) {
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
        printf("MQTT connect failed, status=%d\n", (int)status);
    }
}

void MqttService::on_mqtt_request(void* /*arg*/, err_t result) {
    printf("MQTT request result=%d\n", (int)result);
}

// Kutsutaan kun uusi publish alkaa (topic tiedossa, payloadia ei vielä)
void MqttService::on_incoming_publish(void* arg, const char* topic, u32_t /*tot_len*/) {
    auto* self = static_cast<MqttService*>(arg);
    std::snprintf(self->rx_topic, sizeof(self->rx_topic), "%s", topic ? topic : "");
    self->rx_len = 0;
    self->rx_buf[0] = '\0';
}

// Kutsutaan payload-palojen saapuessa; flags sisältää MQTT_DATA_FLAG_LAST viimeisessä palassa
void MqttService::on_incoming_data(void* arg, const u8_t* data, u16_t len, u8_t flags) {
    auto* self = static_cast<MqttService*>(arg);

    // Kokoa payload bufferiin (trunkkaa tarvittaessa)
    size_t space = (self->RX_MAX - 1) - self->rx_len;
    size_t copy_len = (len < space) ? len : space;

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


    /*if (!topic || !payload) return;

    // Vain cmd-topic käsitellään komentona
    if (std::strcmp(topic, TOPIC_CMD) != 0) return;

    printf("CMD: %s\n", payload);

    // Esimerkkikomennot:
    // - "TOGGLE"
    // - "OPEN"
    // - "CLOSE"
    // - "STATUS"
    //
    // Tässä kohtaa kutsut omaa ovilogikkaasi (rele, moottori, state machine).
    // Kun tila muuttuu, julkaise status.

    if (std::strcmp(payload, "STATUS") == 0) {
        publish(TOPIC_STAT, "OK", 0, true);
        return;
    }

    if (std::strcmp(payload, "TOGGLE") == 0) {
        // TODO: trigger_roller_relay_pulse();
        publish(TOPIC_STAT, "TOGGLING", 0, true);
        return;
    }

    if (std::strcmp(payload, "OPEN") == 0) {
        // TODO: open_door();
        publish(TOPIC_STAT, "OPENING", 0, true);
        return;
    }

    if (std::strcmp(payload, "CLOSE") == 0) {
        // TODO: close_door();
        publish(TOPIC_STAT, "CLOSING", 0, true);
        return;
    }

    publish(TOPIC_STAT, "UNKNOWN_CMD", 0, true);*/
}