#ifndef __DRIVER_LED_H_
#define __DRIVER_LED_H_

#include "config.h"
#include "mcu.h"

#define LED_BLUE_ON LED_1_PIN = 0
#define LED_BLUE_OFF LED_1_PIN = 1

#define LED_BLUE_TOGGLE LED_1_PIN = !LED_1_PIN

#endif /* __DRIVER_LED_H_ */
