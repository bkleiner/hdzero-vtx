#ifndef  _IRS_H_
#define _IRS_H_

extern volatile uint8_t btn1_tflg;
extern volatile uint8_t pwr_sflg;
extern volatile uint8_t pwr_tflg;
extern volatile uint8_t cfg_tflg;
extern volatile uint8_t temp_tflg;
extern volatile uint16_t seconds;
extern volatile uint16_t __idata timer_ms10x;
extern volatile uint8_t timer_4hz;
extern volatile uint8_t timer_8hz;
extern volatile uint8_t timer_16hz;
extern volatile uint8_t RS0_ERR;


void CPU_init(void);

#endif  //_IRS_H_
