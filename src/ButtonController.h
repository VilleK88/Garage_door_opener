//
// Created by ville on 3/3/2026.
//

#ifndef GARAGE_DOOR_OPENER_BUTTONUNIT_H
#define GARAGE_DOOR_OPENER_BUTTONUNIT_H
#include <vector>
#include "IGpioIrqHandler.h"
#include "pico/types.h"


class ButtonController final : public IGpioIrqHandler {
public:
    ButtonController();
    void on_gpio_irq(uint gpio, uint32_t event_mask) override;
private:
    void init() const;

    uint SW0 = 9;
    uint SW1 = 8;
    uint SW2 = 7;
    std::vector<uint> pins{SW0, SW1, SW2};

    static constexpr uint32_t DEBOUNCE_MS = 10;
    uint32_t last_sw0_ms_{0};
    uint32_t last_sw1_ms_{0};
    uint32_t last_sw2_ms_{0};

    bool sw0_down_{false};
    bool sw2_down_{false};
};


#endif //GARAGE_DOOR_OPENER_BUTTONUNIT_H