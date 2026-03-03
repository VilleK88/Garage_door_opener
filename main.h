#ifndef MAIN_H
#define MAIN_H
#include "src/StateMachine.h"
#include <cstdint>

#define DEBOUNCE_MS 10 // Debounce delay in milliseconds

void gpio_callback(uint gpio, uint32_t event_mask);
void network_poll();

#endif