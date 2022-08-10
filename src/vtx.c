#include "vtx.h"

#include "driver/mcu.h"

#include "dm6300.h"

vtx_state_t vtx_state = {
    .cam_mode = CAM_MODE_INVALID,
    .msp_variant = {'B', 'T', 'F', 'L'},
    .font_type = 0,
};

void vtx_pattern_init() {
    mcu_set_720p60();
    mcu_write_reg(0, 0x50, 0x01);
}

void vtx_rf_init(uint8_t channel, uint8_t power) {
    mcu_write_reg(0, 0x8F, 0x00);
    mcu_write_reg(0, 0x8F, 0x01);

    dm6300_init(channel);
    dm6300_set_channel(channel);
    dm6300_set_power(power, channel, 0);

    mcu_write_reg(0, 0x8F, 0x11);
}
