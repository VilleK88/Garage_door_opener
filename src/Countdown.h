#ifndef COUNTDOWN_H
#define COUNTDOWN_H
#pragma once
#include "pico/time.h"

class Countdown {
public:
    Countdown() : end_(nil_time) {}

    Countdown(int ms) {
        countdown_ms(ms);
    }

    void countdown(unsigned int seconds) {
        end_ = make_timeout_time_ms(seconds * 1000);
    }

    void countdown_ms(unsigned int milliseconds) {
        end_ = make_timeout_time_ms(milliseconds);
    }

    bool expired() {
        if (is_nil_time(end_)) return true;
        return time_reached(end_);
    }

    int left_ms() {
        if (is_nil_time(end_)) return 0;

        int64_t us = absolute_time_diff_us(get_absolute_time(), end_);
        if (us <= 0) return 0;
        return (int)(us / 1000);
    }

private:
    absolute_time_t end_;
};


#endif