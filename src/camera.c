#include "camera.h"

#include "driver/i2c_device.h"
#include "driver/mcu.h"
#include "driver/time.h"

#include "debug.h"

void camera_button_init() {
    mcu_write_reg(0, 0x17, 0xC0);
    mcu_write_reg(0, 0x16, 0x00);
    mcu_write_reg(0, 0x15, 0x02);
    mcu_write_reg(0, 0x14, 0x00);
}

void camera_init() {
    camera_button_init();
    debugf("camera init\r\n");
}

camera_mode_t camera_detect() {
    mcu_write_reg(0, 0x8F, 0x91);

    tc3587_init();

    for (uint8_t mode = CAM_MODE_720P50; mode < CAM_MODE_MAX; mode++) {
        switch (mode) {
        case CAM_MODE_720P50:
            debugf("camera trying CAM_MODE_720P50\r\n");
            mcu_set_720p50();
            break;

        case CAM_MODE_720P60:
            debugf("camera trying CAM_MODE_720P60\r\n");
            mcu_set_720p60();
            break;

        case CAM_MODE_720P60_NEW:
            debugf("camera trying CAM_MODE_720P60_NEW\r\n");
            mcu_set_720p60();
            i2c_write16(ADDR_TC3587, 0x0058, 0x00e0);
            break;

        default:
            break;
        }

        time_delay_ms(100);

        uint8_t status_reg = mcu_read_reg(0, 0x02);
        status_reg = status_reg >> 4;

        debugf("camera status_reg: %x\r\n", status_reg);

        if (status_reg == 0) {
            return mode;
        }
    }

    return CAM_MODE_INVALID;
}