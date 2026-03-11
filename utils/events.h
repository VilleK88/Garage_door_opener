//
// Created by ville on 2/25/2026.
//

#ifndef GARAGE_DOOR_OPENER_EVENTS_H
#define GARAGE_DOOR_OPENER_EVENTS_H

#include "pico/util/queue.h"

// Type of event coming from the interrupt callback
typedef enum { EV_SW0, EV_SW1, EV_SW2, EVENT_ENCODER, EV_CALIB, EV_MQTT_CMD } event_type;

// Generic event passed from ISR to main loop through a queue
typedef struct {
    event_type type; // EVENT_BUTTON
    int32_t data; // BUTTON: 1 = press, 0 = release;
    char payload[128];
} event_t;

// Global event queue used by ISR (Interrupt Service Routine) and main loop
extern queue_t events;

constexpr size_t EVENT_QUEUE_SIZE = 512;

#endif //GARAGE_DOOR_OPENER_EVENTS_H