#ifndef __DISPLAY_PORT_H_
#define __DISPLAY_PORT_H_

#include "stdint.h"

#define DP_HEADER0 0x56
#define DP_HEADER1 0x80

void display_port_init();
void display_port_task();

void display_port_send(const uint8_t *data, const uint8_t size);

#endif /* __DISPLAY_PORT_H_ */
