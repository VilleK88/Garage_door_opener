//
// Created by ville on 2/25/2026.
//

#ifndef GARAGE_DOOR_OPENER_EVENTS_H
#define GARAGE_DOOR_OPENER_EVENTS_H

#include "pico/util/queue.h"

// Global event queue used by ISR (Interrupt Service Routine) and main loop
extern queue_t events;

#endif //GARAGE_DOOR_OPENER_EVENTS_H