#ifndef __TIME_H_
#define __TIME_H_

#include "stdint.h"
#include "toolchain.h"

void time_delay_ms(uint16_t ms);

void timer0_isr() INTERRUPT(1);
void timer1_isr() INTERRUPT(3);

#endif /* __TIME_H_ */
