#include "toolchain.h"

#include "driver/i2c.h"
#include "driver/mcu.h"
#include "driver/spi.h"
#include "driver/time.h"
#include "driver/uart.h"

#include "debug.h"
#include "eeprom.h"

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

    while (1) {
        time_delay_ms(500);
        debugf("Hello World!\r\n");
    }
}