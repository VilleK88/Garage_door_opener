#ifndef MAIN_H
#define MAIN_H
#include <vector>
#include <memory>
#include "pico/util/queue.h"
#include "src/StateMachine.h"

#define CLK_DIV 125 // PWM clock divider
#define TOP 999 // PWM counter top value

#define SW0 9 // right button - decreases brightness
#define SW1 8 // middle button - light switch
#define SW2 7 // left button - increases brightness
#define BUTTONS_SIZE 3 // how many buttons
static constexpr std::array<uint, 3> buttons = {SW0, SW1, SW2};

#define LED1 22 // right LED
#define LED2 21 // middle LED
#define LED3 20 // left LED
#define LEDS_SIZE 3 // how many LEDs

#define BR_RATE 50 // step size for brightness changes
#define MAX_BR (TOP + 1) // max brightness
#define BR_MID (MAX_BR / 2) // 50% brightness level
#define LIGHTS_OFF 0
#define LED_OFF_DELAY_MS 1000

#define DEBOUNCE_MS 10 // Debounce delay in milliseconds

static constexpr uint ENC_A = 14;
static constexpr uint ENC_B = 15;

// Type of event coming from the interrupt callback
typedef enum { EV_SW0, EV_SW1, EV_SW2, EVENT_ENCODER } event_type;

// Generic event passed from ISR to main loop through a queue
typedef struct {
    event_type type; // EVENT_BUTTON
    int32_t data; // BUTTON: 1 = press, 0 = release;
} event_t;

void gpio_callback(uint gpio, uint32_t event_mask);
void init_buttons();
void init_encoder();
uint clamp(int br); // returns value between 0 and TOP

#endif