//
// Created by ville on 3/3/2026.
//

#ifndef GARAGE_DOOR_OPENER_BUTTONUNIT_H
#define GARAGE_DOOR_OPENER_BUTTONUNIT_H
#include <cstdint>
#include <vector>

#include "pico/types.h"

constexpr uint SW0 = 9;
constexpr uint SW1 = 8;
constexpr uint SW2 = 7;


class ButtonController {
public:
    ButtonController();
    void init_buttons() const;

    //uint SW0 = 9;
    //uint SW1 = 8;
    //uint SW2 = 7;
private:
    std::vector<uint> pins{SW0, SW1, SW2};
};


#endif //GARAGE_DOOR_OPENER_BUTTONUNIT_H