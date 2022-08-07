#include "debug.h"

#include <stdio.h>

#include "driver/uart.h"

#ifdef DEBUG_MODE

BIT_TYPE verbose = 1;

XDATA_SEG char print_buf[128];

void _debugf(const char *fmt, ...) {
    int len = 0;
    int i = 0;
    va_list ap;

    va_start(ap, fmt);
    len = vsprintf(print_buf, fmt, ap);
    va_end(ap);

    debug_uart_write(print_buf, len);
}

void _verbosef(const char *fmt, ...) {
    int len = 0;
    int i = 0;
    va_list ap;

    if (!verbose) {
        return;
    }

    va_start(ap, fmt);
    len = vsprintf(print_buf, fmt, ap);
    va_end(ap);

    debug_uart_write(print_buf, len);
}

uint8_t _hex_get(uint8_t *data, uint16_t size, uint16_t i) {
    if (i < size) {
        return data[i];
    } else {
        return 0x0;
    }
}

void _debug_hex(uint8_t *data, uint16_t size) {
    for (uint16_t i = 0; i < size;) {
        debugf("%02X: ", i);
        debugf("%02X%02X%02X%02X", _hex_get(data, size, i++), _hex_get(data, size, i++), _hex_get(data, size, i++), _hex_get(data, size, i++));
        debugf(" ");
        debugf("%02X%02X%02X%02X", _hex_get(data, size, i++), _hex_get(data, size, i++), _hex_get(data, size, i++), _hex_get(data, size, i++));
        debugf(" ");
        debugf("%02X%02X%02X%02X", _hex_get(data, size, i++), _hex_get(data, size, i++), _hex_get(data, size, i++), _hex_get(data, size, i++));
        debugf(" ");
        debugf("%02X%02X%02X%02X", _hex_get(data, size, i++), _hex_get(data, size, i++), _hex_get(data, size, i++), _hex_get(data, size, i++));
        debugf("\r\n");
    }
}
#endif