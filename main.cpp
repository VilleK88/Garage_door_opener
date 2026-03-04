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

// lwIP timeout processing (required for networking)
extern "C" {
#include "lwip/timeouts.h"
}

// Global pointers used by interrupt callbacks
RotaryEncoder* g_encoder = nullptr;
ButtonController* g_btnContr = nullptr;

// Global event queue used to pass events from ISR (Interrupt Service Routine) to the main loop
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
    }

    // If Wi-Fi connected, connect to MQTT broker
    if (wifi_conn) {

        // Wait up to 3 seconds for MQTT connection to complete
        mqtt.connect(IP_ADDR, CURRENT_PORT, "picoW-garage");
        const absolute_time_t t_end = make_timeout_time_ms(3000);
        while (!mqtt.is_connected() && absolute_time_diff_us(get_absolute_time(), t_end) < 0) {
            network_poll(); // Pump lwIP / CYW43 events
            sleep_ms(1);
        }
    }

    // Initialize Rotary Encoder
    static RotaryEncoder encoder;
    g_encoder = &encoder;
    // Initialize buttons
    static ButtonController btnContr;
    g_btnContr = &btnContr;
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

/*
 * GPIO interrupt callback.
 * Handles button presses and rotary encoder interrupts.
 * Converts hardware events into event queue messages.
 */
void gpio_callback(uint const gpio, uint32_t const event_mask) {
    // Current timestamp used for debounce logic
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    static bool sw0_down = false;
    static bool sw2_down = false;
    static bool calib_latched = false;

    // Pass GPIO interrupt to rotary encoder handler
    if (g_encoder) {
        g_encoder->on_gpio_irq(gpio, event_mask);
    }

    // --- SW0 button handling ---
    if (gpio == SW0) {
        static uint32_t last_ms_r = 0; // Store last interrupt time
        // Detect button release (rising edge)
        if (event_mask & GPIO_IRQ_EDGE_RISE && now - last_ms_r >= DEBOUNCE_MS) {
            last_ms_r = now;
            sw0_down = false;
            calib_latched = false;
            constexpr event_t event = { .type = EV_SW0, .data = 0 };
            queue_try_add(&events, &event); // Add event to queue
        }
        // Detect button press (falling edge)
        if (event_mask & GPIO_IRQ_EDGE_FALL && now - last_ms_r >= DEBOUNCE_MS) {
            last_ms_r = now;
            sw0_down = true;
            constexpr event_t event = { .type = EV_SW0, .data = 1 };
            queue_try_add(&events, &event); // Add event to queue

            if (sw2_down && !calib_latched) {
                calib_latched = true;
                constexpr event_t ce = { .type = EV_CALIB, .data = 1 };
                queue_try_add(&events, &ce);
            }
        }
    }

    // --- SW1 button handling ---
    if (gpio == SW1) {
        static uint32_t last_ms_m = 0; // Store last interrupt time
        // Detect button release (rising edge)
        if (event_mask & GPIO_IRQ_EDGE_RISE && now - last_ms_m >= DEBOUNCE_MS) {
            last_ms_m = now;
            constexpr event_t event = { .type = EV_SW1, .data = 0 };
            queue_try_add(&events, &event); // Add event to queue
        }
        // Detect button press (falling edge)
        if (event_mask & GPIO_IRQ_EDGE_FALL && now - last_ms_m >= DEBOUNCE_MS){
            last_ms_m = now;
            constexpr event_t event = { .type = EV_SW1, .data = 1 };
            queue_try_add(&events, &event); // Add event to queue
        }
    }

    // --- SW2 button handling ---
    if (gpio == SW2) {
        static uint32_t last_ms_l = 0; // Store last interrupt time
        // Detect button release (rising edge)
        if (event_mask & GPIO_IRQ_EDGE_RISE && now - last_ms_l >= DEBOUNCE_MS) {
            last_ms_l = now;
            sw2_down = false;
            calib_latched = false;
            constexpr event_t event = { .type = EV_SW2, .data = 0 };
            queue_try_add(&events, &event); // Add event to queue
        }
        // Detect button press (falling edge)
        if (event_mask & GPIO_IRQ_EDGE_FALL && now - last_ms_l >= DEBOUNCE_MS) {
            last_ms_l = now;
            sw2_down = true;
            constexpr event_t event = { .type = EV_SW2, .data = 1 };
            queue_try_add(&events, &event); // Add event to queue

            if (sw0_down && !calib_latched) {
                calib_latched = true;
                constexpr event_t ce = { .type = EV_CALIB, .data = 1 };
                queue_try_add(&events, &ce);
            }
        }
    }
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