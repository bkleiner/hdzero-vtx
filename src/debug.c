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
#endif