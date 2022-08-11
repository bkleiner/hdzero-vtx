#include "led.h"

#include "driver/led.h"

void led_init() {
    LED_BLUE_OFF;
}

void led_task() {
    LED_BLUE_ON;
}