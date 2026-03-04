#ifndef MAIN_H
#define MAIN_H
#include "src/StateMachine.h"
#include <cstdint>

// GPIO interrupt callback prototype
void gpio_callback(uint gpio, uint32_t event_mask);
// Network stack polling function
void network_poll();

#endif