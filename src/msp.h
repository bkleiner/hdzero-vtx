#ifndef __MSP_H_
#define __MSP_H_

#include "stdint.h"

#define MSP_START_BYTE '$'
#define MSP1_MAGIC 'M'
#define MSP2_MAGIC 'X'

#define MSP_CMD_FC_VARIANT 0x02
#define MSP_CMD_VTX_STATE 0x58
#define MSP_CMD_STATUS 0x65
#define MSP_CMD_RC 0x69
#define MSP_CMD_DISPLAYPORT 0xB6

void msp_init();
void msp_task();

#endif /* __MSP_H_ */
