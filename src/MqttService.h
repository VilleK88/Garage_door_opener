#ifndef MQTTSERVICE_H
#define MQTTSERVICE_H
#pragma once
#include "main.h"

#include <cstring>

#include "lwip/apps/mqtt.h"
#include "lwip/err.h"

// Lightweight MQTT wrapper for Pico W using lwIP MQTT client.
// - Connects to a broker, publishes/subscribes to topics
// - Receives MQTT messages via lwIP callbacks and forwards them to the main loop as events
class MqttService {
public:
    // Connect to MQTT broker (IP string, port, client id).
    // Returns true if connect request was successfully initiated.
    bool connect(const char* broker_ip, uint16_t port, const char* client_id);

    // Publish a payload to a topic. QoS and retain are optional.
    bool publish(const char* topic, const char* payload, int qos = 0, bool retain = false);

    // Subscribe to a topic with optional QoS.
    bool subscribe(const char* topic, int qos = 0);

    // True if MQTT connection is currently up (set by on_mqtt_connection callback).
    [[nodiscard]] bool is_connected() const { return up; }

    // Periodically attempt to reconnect if disconnected.
    void keep_connection_up();

    // Handle EV_MQTT_CMD events coming from the main event queue.
    // Returns true if the command should trigger a door/state-machine action.
    bool handle_commands(const event_t &event);

    // MQTT topics used by this device.
    static constexpr auto TOPIC_CMD  = "garage/door/cmd";
    static constexpr auto TOPIC_STAT = "garage/door/status";
    static constexpr auto TOPIC_AVAIL = "garage/door/availability";

    // Command decoded from MQTT payload.
    typedef enum { STATUS, TOGGLE, CALIBRATE, UNKNOWN_CMD } cmd_type;
    cmd_type current_cmd{UNKNOWN_CMD};

private:
    // lwIP MQTT connection status callback (called when connect succeeds/fails).
    static void on_mqtt_connection(mqtt_client_t* client, void* arg, mqtt_connection_status_t status);

    // lwIP request completion callback (publish/subscribe results).
    static void on_mqtt_request(void* arg, err_t result);

    // Called when an incoming PUBLISH starts (topic known, payload data follows in chunks).
    static void on_incoming_publish(void* arg, const char* topic, u32_t tot_len);

    // Called for incoming payload chunks. flags includes MQTT_DATA_FLAG_LAST on final chunk.
    static void on_incoming_data(void* arg, const u8_t* data, u16_t len, u8_t flags);

    // Process a complete received MQTT message (topic + full payload).
    void handle_command(const char* topic, const char* payload);

    void reset_buffer();

    mqtt_client_t* client{nullptr}; // lwIP MQTT client instance
    bool up{false}; // connection state

    // Receive buffer for assembling payload fragments from lwIP callbacks.
    static constexpr size_t RX_MAX = 256;
    char rx_topic[128]{}; // last received topic
    char rx_buf[RX_MAX]{}; // assembled payload buffer
    size_t rx_len{0}; // current payload length in rx_buf
};


#endif