#include "osd.h"

#include "driver/time.h"

#include "camera.h"
#include "config.h"
#include "display_port.h"
#include "vtx.h"

uint8_t packet[128];

uint8_t lq_cnt = 0;

const uint8_t cam_mode_id[CAM_MODE_MAX] = {
    0x0,  // CAM_MODE_INVALID
    0x66, // CAM_MODE_720P50
    0x99, // CAM_MODE_720P60
    0xCC, // CAM_MODE_720P60_NEW
};

uint8_t osd_send_metadata_packet() {
    packet[0] = DP_HEADER0;
    packet[1] = DP_HEADER1;
    packet[2] = 0xff;
    packet[3] = 12; // len

    packet[4] = cam_mode_id[vtx_state.cam_mode];

    // msp variant
    packet[5] = vtx_state.msp_variant[0];
    packet[6] = vtx_state.msp_variant[1];
    packet[7] = vtx_state.msp_variant[2];
    packet[8] = vtx_state.msp_variant[3];

    // counter for link quality
    packet[9] = lq_cnt++;

    // VTX temp and overhot
    // temp = pwr_offset >> 1;
    // if (temp > 8)
    //    temp = 8;
    // packet[10] = (heat_protect << 7) | temp;
    packet[11] = vtx_state.font_type;

    packet[12] = VERSION;
    packet[13] = VTX_ID;

    // packet[14] = fc_lock & 0x03;
    // packet[15] = cam_4_3 ? 0xaa : 0x55;

    return 16;
}

void osd_init() {
}

void osd_task() {
    uint8_t len = 0;

    if (timer_8hz) {
        len = osd_send_metadata_packet();
    }

    display_port_send(packet, len);
}