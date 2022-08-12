#include "uart.h"

#include "mcu.h"

volatile uint8_t uart0_rx_buf[UART_BUFFER_SIZE];
volatile uint16_t uart0_rx_head = 0;
volatile uint16_t uart0_rx_tail = 0;

volatile uint8_t uart0_tx_buf[UART_BUFFER_SIZE];
volatile BIT_TYPE uart0_tx_active = 0;
volatile uint16_t uart0_tx_head = 0;
volatile uint16_t uart0_tx_tail = 0;

volatile uint8_t uart1_rx_buf[UART_BUFFER_SIZE];
volatile uint16_t uart1_rx_head = 0;
volatile uint16_t uart1_rx_tail = 0;

volatile uint8_t uart1_tx_buf[UART_BUFFER_SIZE];
volatile BIT_TYPE uart1_tx_active = 0;
volatile uint16_t uart1_tx_head = 0;
volatile uint16_t uart1_tx_tail = 0;

void uart_init() {
}

void uart0_isr() INTERRUPT(4) {
    if (RI0) {
        RI0 = 0;
        const uint16_t next = (uart0_rx_head + 1) % UART_BUFFER_SIZE;
        if (next != uart0_rx_tail) {
            uart0_rx_buf[uart0_rx_head] = SBUF0;
            uart0_rx_head = next;
        }
    }

    if (TI0) {
        TI0 = 0;
        if (uart0_tx_head != uart0_tx_tail) {
            SBUF0 = uart0_tx_buf[uart0_tx_tail];
            uart0_tx_tail = (uart0_tx_tail + 1) % UART_BUFFER_SIZE;
        } else {
            uart0_tx_active = 0;
        }
    }
}

void uart1_isr() INTERRUPT(6) {
    if (RI1) {
        RI1 = 0;
        const uint16_t next = (uart1_rx_head + 1) % UART_BUFFER_SIZE;
        if (next != uart1_rx_tail) {
            uart1_rx_buf[uart1_rx_head] = SBUF1;
            uart1_rx_head = next;
        }
    }

    if (TI1) {
        TI1 = 0;
        if (uart1_tx_head != uart1_tx_tail) {
            SBUF1 = uart1_tx_buf[uart1_tx_tail];
            uart1_tx_tail = (uart1_tx_tail + 1) % UART_BUFFER_SIZE;
        } else {
            uart1_tx_active = 0;
        }
    }
}

uint8_t uart0_read(uint8_t *data) {
    if (uart0_rx_head == uart0_rx_tail) {
        return 0;
    }

    *data = uart0_rx_buf[uart0_rx_tail];
    uart0_rx_tail = (uart0_rx_tail + 1) % UART_BUFFER_SIZE;
    return 1;
}

uint8_t uart0_write_byte(const uint8_t data) {
    if (!uart0_tx_active) {
        // no write in progress, just write out data
        uart0_tx_active = 1;
        SBUF0 = data;
        return 1;
    }

    const uint16_t next = (uart0_tx_head + 1) % UART_BUFFER_SIZE;
    if (next == uart0_tx_tail) {
        return 0;
    }

    uart0_tx_buf[uart0_tx_head] = data;
    uart0_tx_head = next;
    return 1;
}

uint8_t uart0_write(const uint8_t *data, const uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        if (!uart0_write_byte(data[i])) {
            return i;
        }
    }
    return size;
}

uint8_t uart1_read(uint8_t *data) {
    if (uart1_rx_head == uart1_rx_tail) {
        return 0;
    }

    *data = uart1_rx_buf[uart1_rx_tail];
    uart1_rx_tail = (uart1_rx_tail + 1) % UART_BUFFER_SIZE;
    return 1;
}

uint8_t uart1_write_byte(const uint8_t data) {
    if (uart1_tx_active == 0) {
        // no write in progress, just write out data
        uart1_tx_active = 1;
        SBUF1 = data;
        return 1;
    }

    const uint16_t next = (uart1_tx_head + 1) % UART_BUFFER_SIZE;
    if (next == uart1_tx_tail) {
        return 0;
    }

    uart1_tx_buf[uart1_tx_head] = data;
    uart1_tx_head = next;
    return 1;
}

uint8_t uart1_write(const uint8_t *data, const uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        if (!uart1_write_byte(data[i])) {
            return i;
        }
    }
    return size;
}
