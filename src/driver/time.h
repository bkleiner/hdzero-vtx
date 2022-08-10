#ifndef __TIME_H_
#define __TIME_H_

#include "stdint.h"
#include "toolchain.h"

extern volatile IDATA_SEG uint32_t timer_ms;
extern BIT_TYPE timer_8hz;

void timer0_isr() INTERRUPT(1);
void timer1_isr() INTERRUPT(3);

void timer_task();

void time_delay_ms(uint16_t ms);

#endif /* __TIME_H_ */
