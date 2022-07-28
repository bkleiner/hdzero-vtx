#ifndef __MCU_H_
#define __MCU_H_

#include "stdint.h"

#include "toolchain.h"

extern BIT_TYPE btn1_tflg;
extern BIT_TYPE pwr_sflg;
extern BIT_TYPE pwr_tflg;
extern BIT_TYPE cfg_tflg;
extern BIT_TYPE temp_tflg;
extern uint16_t seconds;
extern IDATA_SEG volatile uint16_t timer_ms10x;
extern BIT_TYPE timer_4hz;
extern BIT_TYPE timer_8hz;
extern BIT_TYPE timer_16hz;
extern BIT_TYPE RS0_ERR;

void mcu_init();
void timer_task();
void sa_timer_task();

#endif /* __MCU_H_ */
