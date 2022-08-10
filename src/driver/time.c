#include "time.h"

#include "driver/mcu.h"

volatile IDATA_SEG uint32_t timer_ms = 0;
BIT_TYPE timer_8hz = 0;

#define MS_DELAY_TICKS (2746)

void timer0_isr() INTERRUPT(1) {
    timer_ms++;

    TH0 = 0x6E;
    TL0 = 0xFB;
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

void timer_task() {
    static IDATA_SEG uint32_t last_ms = 0;

    // reset from last run
    timer_8hz = 0;

    if (timer_ms - last_ms > 125) {
        timer_8hz = 1;
        last_ms = timer_ms;
    }
}