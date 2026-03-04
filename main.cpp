#include "pico/stdlib.h"
#include "pico/util/queue.h"

#include "main.h"
#include "src/StateMachine.h"
#include "src/LedController.h"
#include "src/Wifi.h"
#include "config/wifi_config.h"
#include "src/ButtonController.h"
#include "src/Mqttservice.h"
#include "src/RotaryEncoder.h"
#include "utils/events.h"
#include "src/GpioIrqDispatcher.h"

// lwIP timeout processing (required for networking)
extern "C" {
#include "lwip/timeouts.h"
}

// Global event queue used to pass events from ISR (Interrupt Service Routine)
queue_t events;

int main() {
    // Initialize chosen serial port
    stdio_init_all();
    // Initialize event queue for Interrupt Service Routine (ISR)
    queue_init(&events, sizeof(event_t), 128);

    // Create Wi-Fi and MQTT service objects
    Wifi wifi;
    MqttService mqtt;

    // Attempt to initialize and connect Wi-Fi
    bool wifi_conn = false;
    if (wifi.init()) {
        int index = 0;
        // Retry connection up to 5 times
        while (!wifi_conn && index < 5) {
            wifi_conn = wifi.connect_wifi();
            index++;
        }
        // If Wi-Fi connected, connect to MQTT broker
        if (wifi_conn)
            mqtt.connect(IP_ADDR, CURRENT_PORT, "picoW-garage");
    }

    // Initialize Rotary Encoder
    RotaryEncoder encoder;
    GpioIrqDispatcher::register_handler(&encoder);
    // Initialize buttons
    ButtonController btnContr;
    GpioIrqDispatcher::register_handler(&btnContr);
    // Initialize LED controller
    LedController ledContr;
    // Initialize state machine
    StateMachine sm(mqtt, ledContr);

    event_t event;

    while (true) {
        // Maintain network stack and reconnect MQTT if necessary
        if (wifi_conn) {
            network_poll();
            mqtt.keep_connection_up();
        }
        // Process all pending events from the queue
        while (queue_try_remove(&events, &event)) {
            // Update LEDs based on button events
            ledContr.light_switch(event);
            switch (event.type) {
                case EV_CALIB: // Calibration button combination event
                    if (event.data == 1) sm.next_state(CurrentState::init_calib);
                    break;
                case EV_SW1: // Main door control button
                    if (event.data == 1) sm.handle_door();
                    break;
                case EVENT_ENCODER: // Rotary encoder movement event
                    sm.update_position(sm.get_position() + event.data);
                    break;
                case EV_MQTT_CMD: // MQTT command received
                    if (mqtt.handle_commands(event)) {
                        if (mqtt.current_cmd == MqttService::TOGGLE)
                            sm.handle_door();
                        else if (mqtt.current_cmd == MqttService::CALIBRATE)
                            sm.next_state(CurrentState::init_calib);
                    }
                    break;
                default:
                    break;
            }
        }
        // Run state machine logic
        sm.run_sm();
        // Small delay to reduce CPU load
        sleep_ms(1);
    }
}

void gpio_callback(const uint gpio, const uint32_t event_mask) {
    GpioIrqDispatcher::gpio_irq_thunk(gpio, event_mask);
}

/*
 * Poll network stack.
 * Required for Pico W when using lwIP in polling mode.
 * Processes Wi-Fi events and TCP/IP timeouts.
 */
void network_poll() {
    cyw43_arch_poll();
    sys_check_timeouts();
}