#include "osd_display.h"

#include "camera.h"
#include "global.h"
#include "hardware.h"
#include "isr.h"
#include "msp_displayport.h"
#include "version.h"

#define DP_HEADER0 0x56
#define DP_HEADER1 0x80

#define TXBUF_SIZE 69

osd_display_mode_t osd_mode = DISPLAY_OSD;
BIT_TYPE osd_ready = 0;

uint8_t font_type = 0x00;
osd_resolution_t resolution = SD_3016;
uint8_t lq_cnt = 0;

uint8_t osd_buf[HD_VMAX][HD_HMAX];
uint8_t loc_buf[HD_VMAX][7];
uint8_t page_extend_buf[HD_VMAX][7];

uint8_t dptx_buf[256];
uint8_t dptx_rptr = 0;
uint8_t dptx_wptr = 0;

uint8_t dirty_rows[HD_VMAX];
uint8_t row_updates[HD_VMAX];

uint8_t hmax = SD_HMAX;
uint8_t vmax = SD_VMAX;

// buffer for sending data to VRX
uint8_t tx_buf[TXBUF_SIZE];

// TODO: eliminate. global state struct?
extern uint8_t fc_variant[4];
extern uint8_t crc8tab[256];

#ifdef SDCC
void DP_SEND_27M(uint8_t c) {
    // found by trial and error, are on the conservative side
    // could use refinement down the line
    uint16_t __ticks = 300;
    do {
        __ticks--;
    } while (__ticks);

    DP_tx(c);
}
void DP_SEND_20M(uint8_t c) {
    // found by trial and error, are on the conservative side
    // could use refinement down the line
    uint16_t __ticks = 450;
    do {
        __ticks--;
    } while (__ticks);

    DP_tx(c);
}
#else
#define DP_SEND_27M(c)                  \
    {                                   \
        uint8_t _i_;                    \
        for (_i_ = 0; _i_ < 200; _i_++) \
            ;                           \
        DP_tx(c);                       \
    }
#define DP_SEND_20M(c)                  \
    {                                   \
        uint8_t _i_;                    \
        for (_i_ = 0; _i_ < 200; _i_++) \
            ;                           \
        for (_i_ = 0; _i_ < 100; _i_++) \
            ;                           \
        DP_tx(c);                       \
    }
#endif

void dptx_task() {
    while (dptx_wptr != dptx_rptr) {
        if (RF_BW == BW_20M) {
            DP_SEND_20M(dptx_buf[dptx_rptr++]);
        } else {
            DP_SEND_27M(dptx_buf[dptx_rptr++]);
        }
    }
}

uint8_t osd_build_dp_metadata() {
#ifdef USE_TEMPERATURE_SENSOR
    uint8_t temp;
#endif

    tx_buf[0] = DP_HEADER0;
    tx_buf[1] = DP_HEADER1;
    tx_buf[2] = 0xff;
    // len
    tx_buf[3] = 15;

    // camType
    if (CAM_MODE == CAM_720P50)
        tx_buf[4] = 0x66;
    else if (CAM_MODE == CAM_720P60)
        tx_buf[4] = 0x99;
    else if (CAM_MODE == CAM_720P60_NEW)
        tx_buf[4] = 0xAA;
    else if (CAM_MODE == CAM_720P30)
        tx_buf[4] = 0xCC;
    else if (CAM_MODE == CAM_720X540_90)
        tx_buf[4] = 0xEE;
    else
        tx_buf[4] = 0x99;

    // fcType
    if (msp_cmp_fc_variant("QUIC")) {
        // HACK!
        // TODO: remove once another way of selecting font on the VRX is available
        tx_buf[5] = 'A';
        tx_buf[6] = 'R';
        tx_buf[7] = 'D';
        tx_buf[8] = 'U';
    } else {
        tx_buf[5] = fc_variant[0];
        tx_buf[6] = fc_variant[1];
        tx_buf[7] = fc_variant[2];
        tx_buf[8] = fc_variant[3];
    }

    // counter for link quality
    tx_buf[9] = lq_cnt++;

// VTX temp and overhot
#ifdef USE_TEMPERATURE_SENSOR
    temp = pwr_offset >> 1;
    if (temp > 8)
        temp = 8;
    tx_buf[10] = 0x80 | (heat_protect << 6) | temp;
#else
    tx_buf[10] = 0;
#endif

    tx_buf[11] = font_type; // fontType

    tx_buf[12] = 0x00; // deprecated

    tx_buf[13] = VTX_ID;

    tx_buf[14] = fc_lock & 0x03;

    tx_buf[15] = (camRatio == 0) ? 0xaa : 0x55;

    tx_buf[16] = VTX_VERSION_MAJOR;
    tx_buf[17] = VTX_VERSION_MINOR;
    tx_buf[18] = VTX_VERSION_PATCH_LEVEL;

    return 20;
}

uint8_t osd_build_dp_update(uint8_t index) {
    // package struct:
    /*
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
    uint8_t mask[7] = {0};
    uint8_t i, t1;
    uint8_t ptr;
    uint8_t hmax;
    uint8_t len_mask;
    uint8_t page[7] = {0};
    uint8_t page_byte = 0;
    uint8_t num = 0;

    if (resolution == HD_5018) {
        hmax = HD_HMAX;
        len_mask = 7;
        ptr = 11;
    } else {
        ptr = 12;
        len_mask = 4;
        hmax = SD_HMAX;
    }

    // string
    for (i = 0; i < hmax; i++) {
        t1 = osd_buf[index][i];
        if (t1 != ' ') {
            mask[i >> 3] |= (1 << (i & 0x07));
            tx_buf[ptr] = t1;

            page[num >> 3] |= ((page_extend_buf[index][i >> 3] >> (i & 0x07)) & 0x01) << (num & 0x07);
            ptr++;
            num++;
        }
    }

    // page
    page_byte = (num >> 3) + ((num & 0x07) != 0);
    for (i = 0; i < page_byte; i++) {
        if (osd_mode == DISPLAY_OSD)
            tx_buf[ptr++] = page[i];
        else
            tx_buf[ptr++] = 0;
    }

    tx_buf[0] = DP_HEADER0;
    tx_buf[1] = DP_HEADER1;
    tx_buf[2] = (resolution << 5) | index;
    tx_buf[3] = ptr - 4; // len

    // 0x20 flag
    for (i = 0; i < len_mask; i++) {
        tx_buf[4 + i] = mask[i];
    }

    // location_flag
    if (resolution == SD_3016) {
        if (osd_mode == DISPLAY_OSD) {
            tx_buf[8] = loc_buf[index][0];
            tx_buf[9] = loc_buf[index][1];
            tx_buf[10] = loc_buf[index][2];
            tx_buf[11] = loc_buf[index][3];
        } else {
            tx_buf[8] = 0x00; // 0x04;
            tx_buf[9] = 0x00;
            tx_buf[10] = 0x00; // 0x04;
            tx_buf[11] = 0x00;
        }
    }

    return (uint8_t)(ptr + 1);
}

void insert_tx_byte(uint8_t c) {
    dptx_buf[dptx_wptr++] = c;

#ifdef _DEBUG_DISPLAYPORT
    if (dptx_wptr == dptx_rptr) // dptxbuf full
        _outchar('*');
#endif
}

void insert_tx_buf(uint8_t len) {
    uint8_t i;
    uint8_t crc0 = 0, crc1 = 0;

    if (len == 0) {
        return;
    }

    for (i = 0; i < len - 1; i++) {
        crc0 ^= tx_buf[i];
        crc1 = crc8tab[crc1 ^ tx_buf[i]];
        insert_tx_byte(tx_buf[i]);
    }
    insert_tx_byte(crc0);
    insert_tx_byte(crc1);
}

void osd_clear_screen() {
    osd_ready = 0;

    memset(osd_buf, 0x20, sizeof(osd_buf));
    memset(loc_buf, 0x00, sizeof(loc_buf));
    memset(page_extend_buf, 0x00, sizeof(page_extend_buf));
    memset(dirty_rows, 0x00, sizeof(dirty_rows));
    memset(row_updates, 0x00, sizeof(row_updates));
}

void osd_reset() {
    osd_mode = DISPLAY_OSD;
    dptx_wptr = dptx_rptr = 0;

    osd_clear_screen();
}

void osd_write_data(uint8_t row, uint8_t col_start, uint8_t page_extend, uint8_t *data, uint8_t len) {
    uint8_t i = 0;
    uint8_t col = 0;
    uint8_t col_shift = 0;

    osd_ready = 0;
    dirty_rows[row] = 1;

    // only used in low res mode and only set once for a full string
    loc_buf[row][col_start >> 3] |= (1 << (col & 0x07));

    for (i = 0; i < len; i++) {
        col = col_start + i;
        col_shift = col >> 3;

        if (row > vmax || col > hmax) {
            continue;
        }

        osd_buf[row][col] = data[i];
        if (page_extend)
            page_extend_buf[row][col_shift] |= (1 << (col & 0x07));
        else
            page_extend_buf[row][col_shift] &= (0xff - (1 << (col & 0x07)));
    }
}

void osd_init() {
    osd_reset();
}

void osd_set_config(uint8_t font, osd_resolution_t res) {
    font_type = font;

    if (res != resolution) {
        resolution = res;

        if (resolution == HD_5018) {
            vmax = HD_VMAX;
            hmax = HD_HMAX;
        } else {
            vmax = SD_VMAX;
            hmax = SD_HMAX;
        }

        osd_reset();
    }
}

void osd_submit() {
    osd_ready = 1;
}

uint8_t osd_next_row() {
    uint8_t i;

    // check if we have a dirty row
    for (i = 0; i < vmax; i++) {
        if (dirty_rows[i]) {
            dirty_rows[i] = 0;
            row_updates[i] = 0;
            return i;
        }
    }

    // not dirty row found, lets just update everything else
    for (i = 0; i < vmax; i++) {
        if (row_updates[i]) {
            row_updates[i] = 0;
            return i;
        }
    }

    // no row to update left, lets reset and start over
    // by setting eveything but the 0th bit
    // because we gonna update that right away
    memset(row_updates, 1, sizeof(row_updates));
    row_updates[0] = 0;

    return 0;
}

void osd_task() {
    uint8_t len = 0;

    if (timer_8hz) {
        // send param to VRX -- 8HZ
        len = osd_build_dp_metadata();
    } else if (osd_ready) {
        len = osd_build_dp_update(osd_next_row());
    }

    insert_tx_buf(len);
    dptx_task();
}