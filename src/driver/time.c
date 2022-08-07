#include "time.h"

#define MS_DELAY_TICKS (2746)

void timer0_isr() INTERRUPT(1) {
    // TH0 = 138;
}

void timer1_isr() INTERRUPT(3) {
}

void time_delay_ms(uint16_t ms) {
    while (ms--) {
        uint16_t __ticks = MS_DELAY_TICKS; // Measured manually by trial and error
        do {                               // 40 ticks total for this loop
            __ticks--;
        } while (__ticks);
    }
}