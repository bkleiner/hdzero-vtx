#include "toolchain.h"

#include "driver/i2c.h"
#include "driver/mcu.h"
#include "driver/spi.h"
#include "driver/time.h"
#include "driver/uart.h"

#include "camera.h"
#include "debug.h"
#include "dm6300.h"
#include "eeprom.h"

void video_pattern_init() {
    mcu_set_720p60();
    mcu_write_reg(0, 0x50, 0x01);
}

void rf_init(uint8_t channel, uint8_t power) {
    mcu_write_reg(0, 0x8F, 0x00);
    mcu_write_reg(0, 0x8F, 0x01);

    dm6300_init(channel);
    dm6300_set_channel(channel);
    dm6300_set_power(power, channel, 0);

    mcu_write_reg(0, 0x8F, 0x11);
}

void main(void) {
    mcu_init();
    uart_init();

    debugf("========================================================\r\n");
    debugf("     >>>             Divimath DM568X            <<<     \r\n");
    debugf("========================================================\r\n");
    debugf("\r\n");

    spi_init();
    i2c_init();

    eeprom_init();
    eeprom_load();

    camera_init();

    video_pattern_init();
    rf_init(0, 0);

    while (1) {
        time_delay_ms(500);
    }
}