#include "toolchain.h"

#include "driver/i2c.h"
#include "driver/mcu.h"
#include "driver/spi.h"
#include "driver/time.h"
#include "driver/uart.h"

#include "camera.h"
#include "debug.h"
#include "display_port.h"
#include "dm6300.h"
#include "eeprom.h"
#include "osd.h"
#include "vtx.h"

void main(void) {
    mcu_init();
    uart_init();

    debugf("========================================================\r\n");
    debugf("     >>>             Divimath DM568X            <<<     \r\n");
    debugf("========================================================\r\n");
    debugf("\r\n");

    spi_init();
    i2c_init();
    led_init();

    eeprom_init();
    eeprom_load();

    camera_init();
    vtx_state.cam_mode = camera_detect();

    display_port_init();
    osd_init();

    vtx_rf_init(eeprom->vtx.frequency, eeprom->vtx.power);

    while (1) {
        timer_task();

        led_task();
        osd_task();

        display_port_task();
    }
}