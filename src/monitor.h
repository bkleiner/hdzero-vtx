#ifndef __MONITOR_H_
#define __MONITOR_H_

#include "stdint.h"

#include "toolchain.h"

#define Prompt() debugf("\r\nDM568X>")

void Monitor();

#endif /* __MONITOR_H_ */
