#include "display_port.h"

#include "driver/mcu.h"

#include "config.h"
#include "dm6300.h"
#include "util.h"

#define DISPLAY_PORT_BUF_SIZE 0xFF

volatile uint8_t display_port_buf[DISPLAY_PORT_BUF_SIZE];
volatile uint8_t display_port_head = 0;
volatile uint8_t display_port_tail = 0;

void display_port_tx(uint8_t dat) {
    SFR_DATA = dat;
    SFR_CMD = 0x20;
}

void display_port_27m(uint8_t c) {
    // found by trial and error, are on the conservative side
    // could use refinement down the line
    uint16_t __ticks = 300;
    do {
        __ticks--;
    } while (__ticks);

    display_port_tx(c);
}

void display_port_20m(uint8_t c) {
    // found by trial and error, are on the conservative side
    // could use refinement down the line
    uint16_t __ticks = 450;
    do {
        __ticks--;
    } while (__ticks);

    display_port_tx(c);
}

void display_port_init() {
}

void display_port_task() {
    while (display_port_head != display_port_tail) {
        if (DM6300_BW == DM6300_BW_20M) {
            display_port_20m(display_port_buf[display_port_tail++]);
        } else {
            display_port_27m(display_port_buf[display_port_tail++]);
        }
    }
}

uint8_t display_port_write_byte(const uint8_t data) {
    const uint8_t next = (display_port_head + 1);
    if (next == display_port_tail) {
        return 0;
    }

    display_port_buf[display_port_head] = data;
    display_port_head = next;
    return 1;
}

void display_port_send(const uint8_t *data, const uint8_t size) {
    if (size == 0) {
        return;
    }

    uint8_t crc0 = 0, crc1 = 0;
    for (uint8_t i = 0; i < size; i++) {
        const uint8_t byte = data[i];
        crc0 ^= byte;
        crc1 = crc8tab[crc1 ^ byte];
        display_port_write_byte(byte);
    }
    display_port_write_byte(crc0);
    display_port_write_byte(crc1);
}