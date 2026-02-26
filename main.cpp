#include <iostream>
#include <vector>
#include <string>
#include <memory>

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/util/queue.h"

#include "main.h"
#include "src/StateMachine.h"
#include "src/LedController.h"
#include "src/Wifi.h"
#include "config/wifi_config.h"
#include "src/IPStack.h"
#include "src/Mqttservice.h"
#include "src/RotaryEncoder.h"
#include "utils/events.h"

extern "C" {
#include "lwip/timeouts.h"
}

RotaryEncoder* g_encoder = nullptr;

// Global event queue used by ISR (Interrupt Service Routine) and main loop
//queue_t events;

int main() {
    // Initialize chosen serial port
    stdio_init_all();
    // Initialize event queue for Interrupt Service Routine (ISR)
    queue_init(&events, sizeof(event_t), 128);

    Wifi wifi;
    MqttService mqtt;


    bool wifi_conn = false;
    int index = 0;
    while (!wifi_conn && index < 5) {
        wifi_conn = wifi.connect_wifi();
        index++;
    }

    if (wifi_conn) {
        mqtt.connect(MY_IP_ADDR, CURRENT_PORT, "picoW-garage");
        // Pumppaa lwIP timeouts + cyw43, jotta MQTT ehtii viedä handshaken läpi
        const absolute_time_t t_end = make_timeout_time_ms(3000);
        while (!mqtt.is_connected() && absolute_time_diff_us(get_absolute_time(), t_end) < 0) {
            network_poll();
            sleep_ms(1);
        }
    }

    // Initialize Rotary Encoder
    static RotaryEncoder encoder;
    g_encoder = &encoder;
    // Initialize buttons
    init_buttons();
    // Initialize leds
    LedController ledContr;
    // Initialize state machine
    StateMachine sm(mqtt, ledContr);

    event_t event;
    while (true) {
        if (wifi_conn) {
            network_poll();
            mqtt.keep_connection_up();
        }

        while (queue_try_remove(&events, &event)) {
            if (event.type == EV_CALIB && event.data == 1) {
                ledContr.press_button(BtnEv::CalibCombo);
                sm.next_state(CurrentState::init_calib);
            }
            if (event.type == EV_SW1 && event.data == 1) {
                ledContr.press_button(BtnEv::SW1_EV);
                sm.handle_door();
            }
            if (event.type == EVENT_ENCODER) {
                sm.update_position(sm.get_position() + event.data);
            }

            if (event.type == EV_MQTT_CMD) {
                std::cout << "Main got MQTT payload: " << event.payload << "\n";
                if (mqtt.handle_commands(event))
                    sm.handle_door();
            }
        }

        sm.run_sm();
        ledContr.update(to_ms_since_boot(get_absolute_time()));

        sleep_ms(1);
    }
}

void gpio_callback(uint const gpio, uint32_t const event_mask) {
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    static bool sw0_down = false;
    static bool sw2_down = false;
    static bool calib_latched = false;

    if (g_encoder) {
        g_encoder->on_gpio_irq(gpio, event_mask);
    }

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

    // Button press/release with debounce to ensure one physical press counts as one event
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

    /*if (gpio == ENC_A) {
        if (event_mask & GPIO_IRQ_EDGE_RISE) {
            bool b_state = gpio_get(ENC_B);
            event_t ev;
            ev.type = EVENT_ENCODER;
            ev.data = b_state ? -1 : +1;
            queue_try_add(&events, &ev);
        }
    }*/
}

//------------------------------------------------------------
// Configure buttons as inputs and attach IRQ callbacks.
//------------------------------------------------------------
void init_buttons() {
    for (int i = 0; i < BUTTONS_SIZE; i++) {
        gpio_init(buttons[i]); // Initialize GPIO pin
        gpio_set_dir(buttons[i], GPIO_IN); // Set as input
        gpio_pull_up(buttons[i]); // Enable internal pull-up resistor (button reads high = true when not pressed)
        // Configure button interrupt and callback
        if (i == 0) {
            gpio_set_irq_enabled_with_callback(buttons[i], GPIO_IRQ_EDGE_FALL |
            GPIO_IRQ_EDGE_RISE, true, &gpio_callback);
        }
        else {
            gpio_set_irq_enabled(buttons[i], GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
        }
    }
}

void network_poll() {
    cyw43_arch_poll();
    sys_check_timeouts();
}