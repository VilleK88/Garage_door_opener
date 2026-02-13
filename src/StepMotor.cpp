#include "StepMotor.h"

void StepMotor::step(int direction) {
    const int half_step[8][4] = {
        {1, 0, 0, 0},
        {1, 1, 0, 0},
        {0, 1, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 1, 0},
        {0, 0, 1, 1},
        {0, 0, 0, 1},
        {1, 0, 0, 1}
    };

    static int phase = 0;
    phase = phase + direction & 7;
    for (int i = 0; i < )
}