#include "toolchain.h"

#include "driver/i2c.h"
#include "driver/mcu.h"
#include "driver/spi.h"
#include "driver/time.h"
#include "driver/uart.h"

#include "camera.h"
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

    camera_init();

    static camera_mode_t camera_mode = CAM_MODE_INVALID;
    while (1) {
        if (camera_mode == CAM_MODE_INVALID) {
            camera_mode = camera_detect();
        }

        time_delay_ms(500);
    }
}