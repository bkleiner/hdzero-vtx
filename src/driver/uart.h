#ifndef __UART_H_
#define __UART_H_

#include "config.h"
#include "stdint.h"
#include "toolchain.h"

#define _CONCAT(x, y) x##_##y
#define CONCAT(x, y) _CONCAT(x, y)

#define UART_BUFFER_SIZE 512

void uart_init();

void uart0_isr() INTERRUPT(4);
void uart1_isr() INTERRUPT(6);

uint8_t uart0_read(uint8_t *data);
uint8_t uart0_write_byte(const uint8_t data);
uint8_t uart0_write(const uint8_t *data, const uint8_t size);

uint8_t uart1_read(uint8_t *data);
uint8_t uart1_write_byte(const uint8_t data);
uint8_t uart1_write(const uint8_t *data, const uint8_t size);

#define debug_uart_read CONCAT(DEBUG_UART, read)
#define debug_uart_write_byte CONCAT(DEBUG_UART, write_byte)
#define debug_uart_write CONCAT(DEBUG_UART, write)

#define msp_uart_read CONCAT(MSP_UART, read)
#define msp_uart_write_byte CONCAT(MSP_UART, write_byte)
#define msp_uart_write CONCAT(MSP_UART, write)

#endif /* __UART_H_ */
