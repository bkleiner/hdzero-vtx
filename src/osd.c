#include "osd.h"

#include <string.h>

#include "driver/time.h"

#include "camera.h"
#include "config.h"
#include "display_port.h"
#include "vtx.h"

const uint8_t cam_mode_id[CAM_MODE_MAX] = {
    0x0,  // CAM_MODE_INVALID
    0x66, // CAM_MODE_720P50
    0x99, // CAM_MODE_720P60
    0xCC, // CAM_MODE_720P60_NEW
};

BIT_TYPE osd_ready = 0;

uint8_t current_row = 0;
uint8_t display_buf[HD_VMAX][HD_HMAX];
uint8_t location_buf[HD_VMAX][7];
uint8_t page_buf[HD_VMAX][7];

uint8_t hmax = SD_HMAX;
uint8_t vmax = SD_VMAX;

uint8_t packet[128];

uint8_t lq_cnt = 0;

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

/*
packet layout:
{
    uint8_t header0;
    uint8_t header1;
    uint8_t row_number;     // include resolution
    uint8_t length;
    uint8_t *mask;          // not ' ' flag. len:hd(7) sd(4)
    uint8_t *loc_flag;      // only for sd. len:4
    uint8_t *string;
    uint8_t *page;          // length decide by len(string)
    uint8_t crc0;
    uint8_t crc1;

}
*/
uint8_t osd_send_display_port(uint8_t row) {
    uint8_t offset = 0;
    uint8_t len_mask = 0;

    if (vtx_state.resolution == HD_5018) {
        len_mask = 7;
        offset = 11;
    } else {
        len_mask = 4;
        offset = 12;
    }

    uint8_t i = 0;
    uint8_t c = 0;
    uint8_t num = 0;
    uint8_t mask[7] = {0};
    uint8_t page[7] = {0};
    for (i = 0; i < hmax; i++) {
        c = display_buf[row][i];
        if (c == ' ') {
            continue;
        }

        mask[i >> 3] |= (1 << (i & 0x07));
        page[num >> 3] |= ((page_buf[row][i >> 3] >> (i & 0x07)) & 0x01) << (num & 0x07);
        packet[offset] = c;

        offset++;
        num++;
    }

    const uint8_t page_byte = (num >> 3) + ((num & 0x07) != 0);
    for (i = 0; i < page_byte; i++) {
        packet[offset++] = page[i];
    }

    packet[0] = DP_HEADER0;
    packet[1] = DP_HEADER1;
    packet[2] = (vtx_state.resolution << 5) | row;
    packet[3] = offset - 4; // len

    // 0x20 flag
    for (i = 0; i < len_mask; i++) {
        packet[4 + i] = mask[i];
    }

    if (vtx_state.resolution == SD_3016) {
        packet[8] = location_buf[row][0];
        packet[9] = location_buf[row][1];
        packet[10] = location_buf[row][2];
        packet[11] = location_buf[row][3];
    }

    return offset;
}

void osd_clear() {
    memset(display_buf, 0x20, sizeof(display_buf));
    memset(location_buf, 0x00, sizeof(location_buf));
    memset(page_buf, 0x00, sizeof(page_buf));

    osd_ready = 0;
}

void osd_set_config(uint8_t font, osd_resolution_t res) {
    vtx_state.font_type = font;

    if (res != vtx_state.resolution) {
        vtx_state.resolution = res;

        if (vtx_state.resolution == HD_5018) {
            vmax = HD_VMAX;
            hmax = HD_HMAX;
        } else {
            vmax = SD_VMAX;
            hmax = SD_HMAX;
        }

        osd_clear();
    }
}

void osd_write_char(uint8_t row, uint8_t col, uint8_t page_extend, uint8_t data) {
    if (row > vmax || col > hmax) {
        return;
    }

    display_buf[row][col] = data;

    uint8_t col_shift = col >> 3;
    if (page_extend)
        page_buf[row][col_shift] |= (1 << (col & 0x07));
    else
        page_buf[row][col_shift] &= (0xff - (1 << (col & 0x07)));
}

void osd_write_data(uint8_t row, uint8_t col_start, uint8_t page_extend, const uint8_t *data, uint8_t len) {
    osd_ready = 0;

    // only used in low res mode and only set once for a full string
    location_buf[row][col_start >> 3] |= (1 << (col_start & 0x07));

    uint8_t col_end = col_start + len;
    for (uint8_t i = col_start; i < col_end; i++) {
        osd_write_char(row, i, page_extend, data[i]);
    }
}

void osd_submit() {
    current_row = 0;
    osd_ready = 1;
}

void osd_init() {
    osd_clear();
}

void osd_task() {
    uint8_t len = 0;

    if (timer_8hz) {
        len = osd_send_metadata_packet();
    } else if (osd_ready) {
        len = osd_send_display_port(current_row);
        current_row = (current_row + 1) % vmax;
    }

    display_port_send(packet, len);
}