#ifndef __LIFETIME_H_
#define __LIFETIME_H_

#include "common.h"
#include "driver/i2c.h"
#include "global.h"
#include "hardware.h"
#include "isr.h"

void Get_EEP_LifeTime(void);
void Update_EEP_LifeTime(void);
void ParseLifeTime(unsigned char *hourString, unsigned char *minuteString);

#endif /* __LIFETIME_H_ */
