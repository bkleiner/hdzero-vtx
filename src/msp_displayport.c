#include "msp_displayport.h"
#include "camera.h"
#include "common.h"
#include "dm6300.h"
#include "global.h"
#include "hardware.h"
#include "isr.h"
#include "lifetime.h"
#include "print.h"
#include "smartaudio_protocol.h"
#include "spi.h"
#include "uart.h"

uint8_t osd_buf[HD_VMAX][HD_HMAX];
uint8_t loc_buf[HD_VMAX][7];
uint8_t page_extend_buf[HD_VMAX][7];
uint8_t tx_buf[TXBUF_SIZE]; // buffer for sending data to VRX
uint8_t dptxbuf[256];
uint8_t dptx_rptr, dptx_wptr;

uint8_t fc_lock = 0;
uint8_t disp_mode = DISPLAY_OSD; // DISPLAY_OSD | DISPLAY_CMS;
uint8_t osd_ready = 0;

uint8_t fontType = 0x00;
uint8_t resolution = SD_3016;
uint8_t resolution_last = HD_5018;

uint8_t msp_rx_buf[64]; // from FC responding status|variant|rc commands
uint8_t msp_tx_cnt = 0xff;

uint8_t disarmed = 1;

uint8_t vtx_channel;
uint8_t vtx_power;
uint8_t vtx_lp;
uint8_t vtx_pit;
uint8_t vtx_pit_save = PIT_OFF;
uint8_t vtx_offset = 0;
uint8_t first_arm = 0;

uint8_t fc_variant[4] = {'B', 'T', 'F', 'L'};
uint8_t fc_band_rx = 0;
uint8_t fc_channel_rx = 0;
uint8_t fc_pwr_rx = 0;
uint8_t fc_pit_rx = 0;
uint8_t fc_lp_rx = 0;

uint8_t pit_mode_cfg_done = 0;
uint8_t lp_mode_cfg_done = 0;
uint8_t g_IS_ARMED_last = 0;

uint8_t cms_state = CMS_OSD;
uint8_t lq_cnt = 0;

uint8_t mspVtxLock = 0;

#ifdef USE_MSP
void msp_task() {
    uint8_t len;
    static uint8_t tx_row = 0;
    static uint8_t t1 = 0;
    static uint8_t vmax = SD_VMAX;

#ifdef _DEBUG_DISPLAYPORT
    if (RS0_ERR) {
        RS0_ERR = 0;
        _outchar('$'); // RS0 buffer full
    }
#endif
    DP_tx_task();

    // decide by osd_frame size/rate and dptx rate
    if (msp_read_one_frame()) {
        if (resolution == HD_5018) {
            tx_row = HD_VMAX >> 1;
            vmax = HD_VMAX;
        } else {
            tx_row = SD_VMAX;
            vmax = SD_VMAX;
        }
    }

    if (osd_ready) {
        // send osd
        len = get_tx_data_osd(t1);
#ifdef _DEBUG_DISPLAYPORT
// debugf("\n\r%x ", (uint16_t)t1);
#endif
        insert_tx_buf(len);

        t1++;
        if (t1 >= vmax)
            t1 = 0;
    }

    if (timer_4hz)
        timer_4hz = 0;

    // send param to FC -- 8HZ
    // send param to VRX -- 8HZ
    if (timer_8hz) {
        timer_8hz = 0;
        len = get_tx_data_5680();
        insert_tx_buf(len);
        if (dispF_cnt < DISPF_TIME)
            dispF_cnt++;

        if (msp_tx_cnt <= 8)
            msp_tx_cnt++;
        else
            msp_cmd_tx();
    }

    // set_vtx
    set_vtx_param();
}

void msp_parse_vtx_config() {
    uint8_t nxt_ch = 0;
    uint8_t nxt_pwr = 0;
    uint8_t pit_update = 0;
    uint8_t needSaveEEP = 0;
    static uint8_t last_pwr = 255;
    static uint8_t last_lp = 255;
    static uint8_t last_pit = 255;

    if (!(fc_lock & FC_VTX_CONFIG_LOCK)) {
        // first request, set lock and return
        // maybe it could just run anyway?
        fc_lock |= FC_VTX_CONFIG_LOCK;

        fc_pwr_rx = msp_rx_buf[3] - 1;
        if (fc_pwr_rx > POWER_MAX + 2)
            fc_pwr_rx = 0;

        return;
    }

    fc_band_rx = msp_rx_buf[1];
    fc_channel_rx = msp_rx_buf[2];
    fc_pwr_rx = msp_rx_buf[3];
    fc_pit_rx = msp_rx_buf[4];
    fc_lp_rx = msp_rx_buf[8];

#ifdef _DEBUG_MODE
    debugf("\r\nparseMspVtx_V2");
    debugf("\r\n    fc_vtx_dev:    %x", (uint16_t)msp_rx_buf[0]);
    debugf("\r\n    fc_band_rx:    %x", (uint16_t)msp_rx_buf[1]);
    debugf("\r\n    fc_channel_rx: %x", (uint16_t)msp_rx_buf[2]);
    debugf("\r\n    fc_pwr_rx:     %x", (uint16_t)msp_rx_buf[3]);
    debugf("\r\n    fc_pit_rx :    %x", (uint16_t)msp_rx_buf[4]);
    debugf("\r\n    fc_vtx_status: %x", (uint16_t)msp_rx_buf[7]);
    debugf("\r\n    fc_lp_rx:      %x", (uint16_t)msp_rx_buf[8]);
    debugf("\r\n    fc_bands:      %x", (uint16_t)msp_rx_buf[12]);
    debugf("\r\n    fc_channels:   %x", (uint16_t)msp_rx_buf[13]);
    debugf("\r\n    fc_powerLevels %x", (uint16_t)msp_rx_buf[14]);
#endif

    if (SA_lock)
        return;

    mspVtxLock |= 1;

    // update LP_MODE
    if (fc_lp_rx != last_lp) {
        last_lp = fc_lp_rx;
        if (fc_lp_rx < 2) {
            LP_MODE = fc_lp_rx;
        }
        needSaveEEP = 1;
    }
    // update channel
    if (fc_band_rx == 5) // race band
        nxt_ch = fc_channel_rx - 1;
    else if (fc_band_rx == 4) { // fashark band
        if (fc_channel_rx == 2)
            nxt_ch = 8;
        else if (fc_channel_rx == 4)
            nxt_ch = 9;
    }
    if (RF_FREQ != nxt_ch) {
        vtx_channel = nxt_ch;
        RF_FREQ = nxt_ch;
        DM6300_SetChannel(RF_FREQ);
        needSaveEEP = 1;
    }

    // update pit
    nxt_pwr = fc_pwr_rx - 1;

    if (fc_pit_rx != last_pit) {
        PIT_MODE = fc_pit_rx & 1;
#ifdef _DEBUG_MODE
        debugf("\r\nPIT_MODE = %x", (uint16_t)PIT_MODE);
#endif
        if (PIT_MODE) {
            DM6300_SetPower(POWER_MAX + 1, RF_FREQ, pwr_offset);
            cur_pwr = POWER_MAX + 1;
        } else {
#ifndef VIDEO_PAT
#ifdef VTX_L
            if ((RF_POWER == 3) && (!g_IS_ARMED))
                pwr_lmt_done = 0;
            else
#endif
#endif
                if (nxt_pwr == POWER_MAX + 1) {
                WriteReg(0, 0x8F, 0x10);
                cur_pwr = POWER_MAX + 2;
                vtx_pit_save = PIT_0MW;
                temp_err = 1;
            } else {
                DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                cur_pwr = RF_POWER;
            }
        }
        last_pit = fc_pit_rx;
        vtx_pit_save = PIT_MODE;
        needSaveEEP = 1;
    }

    // update power
    if (last_pwr != nxt_pwr) {
        if (last_pwr == POWER_MAX + 1) {
            // Exit 0mW
            if (cur_pwr == POWER_MAX + 2) {
                if (PIT_MODE)
                    Init_6300RF(RF_FREQ, POWER_MAX + 1);
                else
                    Init_6300RF(RF_FREQ, RF_POWER);
                vtx_pit_save = PIT_MODE;
                needSaveEEP = 1;
            }
        } else if (nxt_pwr == POWER_MAX + 1) {
            // Enter 0mW
            if (cur_pwr != POWER_MAX + 2) {
                WriteReg(0, 0x8F, 0x10);
                cur_pwr = POWER_MAX + 2;
                vtx_pit_save = PIT_0MW;
                temp_err = 1;
            }
        } else if (nxt_pwr <= POWER_MAX) {
            RF_POWER = nxt_pwr;
            if (PIT_MODE)
                RF_POWER = POWER_MAX + 1;

            if (rf_init_done) {
                if (cur_pwr != RF_POWER) {
#ifndef VIDEO_PAT
#ifdef VTX_L
                    if ((RF_POWER == 3) && (!g_IS_ARMED))
                        pwr_lmt_done = 0;
                    else
#endif
#endif
                    {
                        DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                        cur_pwr = RF_POWER;
                    }
                    needSaveEEP = 1;
                }
            } else {
                Init_6300RF(RF_FREQ, RF_POWER);
                DM6300_AUXADC_Calib();
            }
        }
        last_pwr = nxt_pwr;
    }

    if (needSaveEEP)
        Setting_Save();

#ifdef _DEBUG_MODE
    debugf("\r\nparseMspVtx_V2 pwr:%x, pit:%x", (uint16_t)nxt_pwr, (uint16_t)fc_pit_rx);
#endif
}

void clear_screen() {
    memset(osd_buf, 0x20, sizeof(osd_buf));
    memset(loc_buf, 0x00, sizeof(loc_buf));
    memset(page_extend_buf, 0x00, sizeof(page_extend_buf));
}

void write_string(uint8_t rx, uint8_t row, uint8_t col, uint8_t page_extend) {
    if (disp_mode == DISPLAY_OSD) {
        if (resolution == HD_5018) {
            if (row < HD_VMAX && col < HD_HMAX) {
                osd_buf[row][col] = rx;
                if (page_extend)
                    page_extend_buf[row][col >> 3] |= (1 << (col & 0x07));
                else
                    page_extend_buf[row][col >> 3] &= (0xff - (1 << (col & 0x07)));
            }
        } else if (resolution == SD_3016 || resolution == HD_3016) {
            if (row < SD_VMAX && col < SD_HMAX) {
                osd_buf[row][col] = rx;
                if (page_extend)
                    page_extend_buf[row][col >> 3] |= (1 << (col & 0x07));
                else
                    page_extend_buf[row][col >> 3] &= (0xff - (1 << (col & 0x07)));
            }
        }
    }
}

void mark_loc(uint8_t row, uint8_t col) {
    loc_buf[row][col >> 3] |= (1 << (col & 0x07));
}

uint8_t msp_parse_displayport(uint8_t len) {
    uint8_t row = 0, col = 0;
    uint8_t page_extend = 0;
    uint8_t ptr = 0;

    if (len == 0)
        return 0;

    switch (msp_rx_buf[0]) {
    case SUBCMD_CLEAR:
        osd_ready = 0;

        if (disp_mode == DISPLAY_OSD)
            clear_screen();

#ifdef _DEBUG_DISPLAYPORT
        _outchar('\r');
        _outchar('\n');
        _outchar('C');
#endif
        return 0;

    case SUBCMD_CONFIG:
        fontType = msp_rx_buf[1];
        resolution = msp_rx_buf[2];

        if (resolution != resolution_last)
            fc_init();

        resolution_last = resolution;
        return 0;

    case SUBCMD_DRAW:
#ifdef _DEBUG_DISPLAYPORT
        _outchar('D');
        _outchar(' ');
#endif
        if (!(fc_lock & FC_OSD_LOCK)) {
            Flicker_LED(8);
            fc_lock |= FC_OSD_LOCK;
        }

        osd_ready = 1;
        return 1;

    case SUBCMD_WRITE:
        osd_ready = 0;

#ifdef _DEBUG_DISPLAYPORT
        _outchar('W');
#endif

        row = msp_rx_buf[1];
        col = msp_rx_buf[2];
        page_extend = msp_rx_buf[3] & 0x01;

        if (resolution == HD_3016) {
            row -= 1;
            col -= 10;
        }
        mark_loc(row, col);

        for (ptr = 0; ptr < len; ptr++) {
            write_string(msp_rx_buf[4 + ptr], row, col + ptr, page_extend);
        }
        break;

    default:
    case SUBCMD_RELEASE:
    case SUBCMD_HEARTBEAT:
        return 0;
    }

    return 0;
}

void msp_parse_rc() {
    uint16_t roll, pitch, yaw, throttle;

    if (!(fc_lock & FC_RC_LOCK))
        fc_lock |= FC_RC_LOCK;

    roll = (msp_rx_buf[1] << 8) | msp_rx_buf[0];
    pitch = (msp_rx_buf[3] << 8) | msp_rx_buf[2];
    yaw = (msp_rx_buf[5] << 8) | msp_rx_buf[4];
    throttle = (msp_rx_buf[7] << 8) | msp_rx_buf[6];

    update_cms_menu(roll, pitch, yaw, throttle);
}

uint8_t msp_handle_cmd(uint16_t cmd, uint16_t payload_len) {
    uint8_t i;
    uint8_t ret = 0;

    switch (cmd) {
    case MSP_CMD_FC_VARIANT:
        if (!(fc_lock & FC_VARIANT_LOCK))
            fc_lock |= FC_VARIANT_LOCK;

        for (i = 0; i < 4; i++)
            fc_variant[i] = msp_rx_buf[i];
        break;

    case MSP_CMD_STATUS_BYTE:
        if (!(fc_lock & FC_STATUS_LOCK))
            fc_lock |= FC_STATUS_LOCK;

        g_IS_ARMED = (msp_rx_buf[6] & 0x01);
        g_IS_PARALYZE = (msp_rx_buf[9] & 0x80);
        disarmed = !g_IS_ARMED;
        break;

    case MSP_CMD_RC_BYTE:
        msp_parse_rc();
        break;

    case MSP_CMD_DISPLAYPORT_BYTE:
        ret = msp_parse_displayport(payload_len);
        break;

    case MSP_CMD_VTX_CONFIG:
        msp_parse_vtx_config();
        break;

    default:
        // unknown command do nothing
        break;
    }

    if ((fc_lock & FC_VARIANT_LOCK) && (fc_lock & FC_VTX_CONFIG_LOCK) && !(fc_lock & FC_INIT_VTX_TABLE_LOCK)) {
        fc_lock |= FC_INIT_VTX_TABLE_LOCK;
        if (fc_variant[0] == 'B' && fc_variant[1] == 'T' && fc_variant[2] == 'F' && fc_variant[3] == 'L') {
#ifdef INIT_VTX_TABLE
            InitVtxTable();
#endif
        }
    }

    return ret;
}

uint8_t msp_read_one_frame() {
    static uint8_t state = MSP_HEADER_START;
    static uint8_t ptr = 0; // write ptr of msp_rx_buf
    static uint8_t crc = 0;

    static uint16_t cmd = 0;
    static uint16_t length = 0;
    static uint16_t payload_len = 0;

    uint8_t ret = 0;
    uint8_t full_frame = 0;
    uint8_t i, rx;

    for (i = 0; i < 16; i++) {
        if ((!CMS_ready()) || full_frame)
            return ret;

        rx = CMS_rx();

        switch (state) {
        case MSP_HEADER_START:
            if (rx == MSP_HEADER_START_BYTE) {
                ptr = 0;
                state = MSP_HEADER_M;
            }
#ifdef _DEBUG_DISPLAYPORT
            else
                _outchar('&');
#endif
            break;

        case MSP_HEADER_M:
            if (rx == MSP_HEADER_M_BYTE) {
                state = MSP_PACKAGE_REPLAY1;
            } else if (rx == MSP_HEADER_M2_BYTE) {
                state = MSP_PACKAGE_REPLAY2;
            } else {
                state = MSP_HEADER_START;
            }
            break;

        case MSP_PACKAGE_REPLAY1: // 0x3E
            if (rx == MSP_PACKAGE_REPLAY_BYTE) {
                state = MSP_LENGTH;
            } else {
                state = MSP_HEADER_START;
            }
            break;

        case MSP_LENGTH:
            crc = rx;
            state = MSP_CMD;
            length = rx;
            payload_len = length - 4;
            break;

        case MSP_CMD:
            crc ^= rx;
            cmd = rx;
            ptr = 0;
            if (length == 0) {
                state = MSP_CRC1;
            } else {
                state = MSP_RX1;
            }
            break;

        case MSP_RX1:
            crc ^= rx;
            msp_rx_buf[ptr++] = rx;
            ptr &= 63;
            length--;
            if (length == 0)
                state = MSP_CRC1;
            break;

        case MSP_CRC1:
            if (rx == crc) {
                ret = msp_handle_cmd(cmd, payload_len);
            }
#ifdef _DEBUG_DISPLAYPORT
            else {
                _outchar('^');
            }
#endif
            full_frame = 1;
            state = MSP_HEADER_START;
            break;

        case MSP_PACKAGE_REPLAY2: // 0x3E
            if (rx == MSP_PACKAGE_REPLAY_BYTE) {
                state = MSP_ZERO;
            } else {
                state = MSP_HEADER_START;
            }
            break;

        case MSP_ZERO:         // 0x00
            crc = crc8tab[rx]; // 0 ^ rx = rx
            if (rx == 0x00) {
                state = MSP_CMD_L;
            } else {
                state = MSP_HEADER_START;
            }
            break;

        case MSP_CMD_L:
            crc = crc8tab[crc ^ rx];
            cmd = rx;
            state = MSP_CMD_H;
            break;

        case MSP_CMD_H:
            crc = crc8tab[crc ^ rx];
            cmd += ((uint16_t)rx << 8);
            state = MSP_LEN_L;
            break;

        case MSP_LEN_L:
            crc = crc8tab[crc ^ rx];
            length = rx;
            state = MSP_LEN_H;
            break;

        case MSP_LEN_H:
            crc = crc8tab[crc ^ rx];
            length += ((uint16_t)rx << 8);
            payload_len = length;
            ptr = 0;
            state = MSP_RX2;
            break;

        case MSP_RX2:
            crc = crc8tab[crc ^ rx];
            msp_rx_buf[ptr++] = rx;
            ptr &= 63;
            length--;
            if (length == 0)
                state = MSP_CRC2;
            break;

        case MSP_CRC2:
            if (crc == rx) {
                ret = msp_handle_cmd(cmd, payload_len);
            }
#ifdef _DEBUG_MODE
            debugf("\r\ncrc : %x,%x", (uint16_t)crc, (uint16_t)rx);
#endif
            full_frame = 1;
            state = MSP_HEADER_START;
            break;
        }
    }

    return ret;
}

void init_tx_buf() {
    uint8_t i;
    for (i = 0; i < TXBUF_SIZE; i++)
        tx_buf[i] = 0;
}

void fc_init() {
    disp_mode = DISPLAY_OSD;
    dptx_wptr = dptx_rptr = 0;

    osd_ready = 0;
    clear_screen();
    init_tx_buf();
    // vtx_menu_init();
}

uint8_t get_tx_data_5680() // prepare data to VRX
{
    uint8_t temp;

    tx_buf[0] = DP_HEADER0;
    tx_buf[1] = DP_HEADER1;
    tx_buf[2] = 0xff;
    // len
    tx_buf[3] = 12;

    // camType
    if (CAM_MODE == CAM_720P50)
        tx_buf[4] = 0x66;
    else if (CAM_MODE == CAM_720P60)
        tx_buf[4] = 0x99;
    else if (CAM_MODE == CAM_720P60_NEW)
        tx_buf[4] = 0xAA;
    else if (CAM_MODE == CAM_720P30)
        tx_buf[4] = 0xCC;

    // fcType
    tx_buf[5] = fc_variant[0];
    tx_buf[6] = fc_variant[1];
    tx_buf[7] = fc_variant[2];
    tx_buf[8] = fc_variant[3];

    // counter for link quality
    tx_buf[9] = lq_cnt++;

    // VTX temp and overhot
    temp = pwr_offset >> 1;
    if (temp > 8)
        temp = 8;
    tx_buf[10] = (heat_protect << 7) | temp;

    tx_buf[11] = fontType; // fontType

    tx_buf[12] = VERSION;

#if defined VTX_B
    tx_buf[13] = VTX_B_ID; // reversed
#elif defined VTX_M
    tx_buf[13] = VTX_M_ID;
#elif defined VTX_S
    tx_buf[13] = VTX_S_ID;
#elif defined VTX_R
    tx_buf[13] = VTX_R_ID;
#elif defined VTX_WL
    tx_buf[13] = VTX_WL_ID;
#elif defined VTX_L
    tx_buf[13] = VTX_L_ID;
#endif

    tx_buf[14] = fc_lock & 0x03;

    tx_buf[15] = cam_4_3 ? 0xaa : 0x55;

    tx_buf[16] = 0x40; // crc

    return 17;
}

uint8_t get_tx_data_osd(uint8_t index) // prepare osd+data to VTX
{
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
        if (disp_mode == DISPLAY_OSD)
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
        if (disp_mode == DISPLAY_OSD) {
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
    dptxbuf[dptx_wptr++] = c;
#ifdef _DEBUG_DISPLAYPORT
    if (dptx_wptr == dptx_rptr) // dptxbuf full
        _outchar('*');
#endif
}

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

void DP_tx_task() {
    uint8_t i;
    for (i = 0; i < 32; i++) {
        if (dptx_wptr != dptx_rptr) {
#if (1)
            if (RF_BW == BW_20M) {
                DP_SEND_20M(dptxbuf[dptx_rptr++]);
            } else {
                DP_SEND_27M(dptxbuf[dptx_rptr++]);
            }
#else
            _outchar(dptxbuf[dptx_rptr++]);
#endif
        } else
            break;
    }
}

void insert_tx_buf(uint8_t len) {
    uint8_t i;
    uint8_t crc0, crc1;

    crc0 = 0;
    crc1 = 0;
    for (i = 0; i < len - 1; i++) {
        crc0 ^= tx_buf[i];
        crc1 = crc8tab[crc1 ^ tx_buf[i]];
        insert_tx_byte(tx_buf[i]);
    }
    insert_tx_byte(crc0);
    insert_tx_byte(crc1);
}

void msp_send_header(uint8_t dl) {
    if (dl)
        WAIT(20);

    CMS_tx(0x24);
    CMS_tx(0x4d);
    CMS_tx(0x3c);
}

void msp_cmd_tx() // send 3 commands to FC
{
    uint8_t i, j;
    uint8_t msp_cmd[4] = {
        0x02, // msp_fc_variant
        0x65, // msp_status
        0x69, // msp_rc
        0x58  // msp_vtx_config
    };

    if (fc_lock & FC_VTX_CONFIG_LOCK)
        j = 3;
    else
        j = 4;

    for (i = 0; i < j; i++) {
        msp_send_header(0);
        CMS_tx(0x00);
        CMS_tx(msp_cmd[i]);
        CMS_tx(msp_cmd[i]);
    }
}

void msp_eeprom_write() {
    msp_send_header(1);
    CMS_tx(0x00);
    CMS_tx(250);
    CMS_tx(250);
}

void msp_set_vtx_config(uint8_t power, uint8_t save) {
    uint8_t crc = 0;
    uint8_t channel = 0;
    uint8_t band;

    if (RF_FREQ < 8) {
        band = 5;
        channel = RF_FREQ;
    } else {
        band = 4;
        if (RF_FREQ == 8)
            channel = 1;
        else if (RF_FREQ == 9)
            channel = 3;
    }
    msp_send_header(0);
    CMS_tx(0x0f);
    crc ^= 0x0f; // len
    CMS_tx(0x59);
    crc ^= 0x59; // cmd
    CMS_tx(0x00);
    crc ^= 0x00; // freq_h
    CMS_tx(0x00);
    crc ^= 0x00; // freq_l
    CMS_tx(power + 1);
    crc ^= (power + 1); // power_level
    CMS_tx((PIT_MODE & 1));
    crc ^= (PIT_MODE & 1); // pitmode
    CMS_tx(LP_MODE);
    crc ^= LP_MODE; // lp_mode
    CMS_tx(0x00);
    crc ^= 0x00; // pit_freq_h
    CMS_tx(0x00);
    crc ^= 0x00; // pit_freq_l
    CMS_tx(band);
    crc ^= band; // band
    CMS_tx(channel + 1);
    crc ^= (channel + 1); // channel
    CMS_tx(0x00);
    crc ^= 0x00; // freq_h
    CMS_tx(0x00);
    crc ^= 0x00; // freq_l
    CMS_tx(0x06);
    crc ^= 0x06; // band number
    CMS_tx(0x08);
    crc ^= 0x08; // channel number
#ifdef VTX_L
    if (powerLock) {
        CMS_tx(3);
        crc ^= (3); // power number
    } else {
        CMS_tx(5);
        crc ^= (5); // power number
    }
#else
    CMS_tx(POWER_MAX + 2);
    crc ^= (POWER_MAX + 2); // power number
#endif
    CMS_tx(0x00);
    crc ^= 0x00; // disable table
    CMS_tx(crc);

#ifdef _DEBUG_MODE
    debugf("\r\nmsp_set_vtx_config:F%x,P%x,M:%x", (uint16_t)RF_FREQ, (uint16_t)power, (uint16_t)PIT_MODE);
#endif

    if (save)
        msp_eeprom_write();
}

void update_cms_menu(uint16_t roll, uint16_t pitch, uint16_t yaw, uint16_t throttle) {
    /*
     *                throttle(油门) +                                          pitch(俯仰) +
     *
     *
     *   yaw(方向) -                      yaw(方向) +               roll(横滚) -                    roll(横滚) +
     *
     *
     *                throttle(油门) -                                          pitch(俯仰) -
     */
    static uint8_t vtx_state = 0;
    uint8_t mid = 0;
    static uint8_t last_mid = 1;
    static uint8_t cms_cnt;

    uint8_t VirtualBtn = BTN_INVALID;

    uint8_t IS_HI_yaw = IS_HI(yaw);
    uint8_t IS_LO_yaw = IS_LO(yaw);
    uint8_t IS_MID_yaw = IS_MID(yaw);

    uint8_t IS_HI_throttle = IS_HI(throttle);
    uint8_t IS_LO_throttle = IS_LO(throttle);
    // uint8_t IS_MID_throttle = IS_MID(throttle);

    uint8_t IS_HI_pitch = IS_HI(pitch);
    uint8_t IS_LO_pitch = IS_LO(pitch);
    uint8_t IS_MID_pitch = IS_MID(pitch);

    uint8_t IS_HI_roll = IS_HI(roll);
    uint8_t IS_LO_roll = IS_LO(roll);
    uint8_t IS_MID_roll = IS_MID(roll);

    if (!disarmed && (cms_state != CMS_OSD)) {
        fc_init();
        cms_state = CMS_OSD;
    }

    // btn control
    if (IS_MID_yaw && IS_MID_roll && IS_MID_pitch) {
        mid = 1;
        VirtualBtn = BTN_MID;
    } else {
        mid = 0;
        if (IS_HI_yaw && IS_MID_roll && IS_MID_pitch)
            VirtualBtn = BTN_ENTER;
        else if (IS_LO_yaw && IS_MID_roll && IS_MID_pitch)
            VirtualBtn = BTN_EXIT;
        else if (IS_MID_yaw && IS_MID_roll && IS_HI_pitch)
            VirtualBtn = BTN_UP;
        else if (IS_MID_yaw && IS_MID_roll && IS_LO_pitch)
            VirtualBtn = BTN_DOWN;
        else if (IS_MID_yaw && IS_LO_roll && IS_MID_pitch)
            VirtualBtn = BTN_LEFT;
        else if (IS_MID_yaw && IS_HI_roll && IS_MID_pitch)
            VirtualBtn = BTN_RIGHT;
        else
            VirtualBtn = BTN_INVALID;
    }

    switch (cms_state) {
    case CMS_OSD:
        if (disarmed) {
            if (IS_HI_yaw && IS_LO_throttle && IS_LO_roll && IS_LO_pitch) {
                if (cur_pwr == POWER_MAX + 2) {
                    cms_state = CMS_EXIT_0MW;
                    // debugf("\r\ncms_state(%x),cur_pwr(%x)",cms_state, cur_pwr);
                    cms_cnt = 0;
                    break;
                }
                /*if(!SA_lock)*/ {
                    cms_state = CMS_ENTER_VTX_MENU;
                    // debugf("\r\ncms_state(%x)",cms_state);
                    vtx_menu_init();
                    vtx_state = 0;
                }
            } else if (IS_LO_yaw && IS_LO_throttle && IS_HI_roll && IS_LO_pitch) {
                if (cur_pwr != POWER_MAX + 2) {
                    cms_state = CMS_ENTER_0MW;
                    // debugf("\r\ncms_state(%x)",cms_state);
                    cms_cnt = 0;
                }
            } else if (VirtualBtn == BTN_ENTER) {
                cms_state = CMS_ENTER_CAM;
                // debugf("\r\ncms_state(%x)",cms_state);
                cms_cnt = 0;
            } else
                cms_cnt = 0;
        }
        // debugf("\n\r CMS_OSD");
        break;

    case CMS_ENTER_0MW:
        if (IS_LO_yaw && IS_LO_throttle && IS_HI_roll && IS_LO_pitch)
            cms_cnt++;
        else {
            cms_state = CMS_OSD;
        }
        if (cms_cnt == 3) {
            vtx_channel = RF_FREQ;
            vtx_power = RF_POWER;
            vtx_lp = LP_MODE;
            PIT_MODE = PIT_0MW;
            vtx_pit = PIT_0MW;
            if (!SA_lock) {
                save_vtx_param();
            } else {
                msp_set_vtx_config(POWER_MAX + 1, 0); // enter 0mW for SA
                WriteReg(0, 0x8F, 0x10);
                cur_pwr = POWER_MAX + 2;
                temp_err = 1;
            }
        }
        break;

    case CMS_EXIT_0MW:
        if (vtx_pit == PIT_0MW) {
            vtx_channel = RF_FREQ;
            vtx_power = RF_POWER;
            vtx_lp = LP_MODE;
            vtx_pit = PIT_OFF;
            vtx_offset = OFFSET_25MW;
            if (SA_lock) {
                // PIT_MODE = 2;
                vtx_pit = PIT_P1MW;
                Init_6300RF(RF_FREQ, POWER_MAX + 1);
                cur_pwr = POWER_MAX + 1;
                DM6300_AUXADC_Calib();
#ifdef _DEBUG_DISPLAYPORT
                debugf("\r\nExit 0mW\r\n");
#endif
                // debugf("\r\n exit0");
            } else {
                save_vtx_param();
                pit_mode_cfg_done = 1; // avoid to config DM6300 again
                // SPI_Write(0x6, 0xFF0, 0x00000018);
                // SPI_Write(0x3, 0xd00, 0x00000003);
                Init_6300RF(RF_FREQ, RF_POWER);
                DM6300_AUXADC_Calib();
#ifdef _DEBUG_MODE
                debugf("\r\nExit 0mW\r\n");
#endif
                // debugf("\r\n exit0");
            }
        }

        if (!(IS_HI_yaw && IS_LO_throttle && IS_LO_roll && IS_LO_pitch))
            cms_state = CMS_OSD;
        break;

    case CMS_ENTER_VTX_MENU: {
        cms_state = CMS_VTX_MENU;
        vtx_menu_init();
        break;
    }

    case CMS_VTX_MENU: {
        if (disarmed) {
            if (last_mid) {
                switch (vtx_state) {
                // channel
                case 0:
                    if (VirtualBtn == BTN_DOWN)
                        vtx_state = 1;
                    else if (VirtualBtn == BTN_UP)
                        vtx_state = 6;
                    else if (VirtualBtn == BTN_RIGHT) {
                        if (SA_lock == 0) {
                            vtx_channel++;
                            if (vtx_channel == FREQ_MAX_EXT + 1)
                                vtx_channel = 0;
                        }
                    } else if (VirtualBtn == BTN_LEFT) {
                        if (SA_lock == 0) {
                            vtx_channel--;
                            if (vtx_channel > FREQ_MAX_EXT + 1)
                                vtx_channel = FREQ_MAX_EXT;
                        }
                    }
                    update_vtx_menu_param(vtx_state);
                    break;

                // power
                case 1:
                    if (VirtualBtn == BTN_DOWN)
                        vtx_state = 2;
                    else if (VirtualBtn == BTN_UP)
                        vtx_state = 0;
                    else if (VirtualBtn == BTN_RIGHT) {
                        if (SA_lock == 0) {
                            vtx_power++;
#ifdef VTX_L
                            if (powerLock)
                                vtx_power &= 0x01;
#endif
                            if (vtx_power > POWER_MAX)
                                vtx_power = 0;
                        }
                    } else if (VirtualBtn == BTN_LEFT) {
                        if (SA_lock == 0) {
                            vtx_power--;
#ifdef VTX_L
                            if (powerLock)
                                vtx_power &= 0x01;
#endif
                            if (vtx_power > POWER_MAX)
                                vtx_power = POWER_MAX;
                        }
                    }
                    update_vtx_menu_param(vtx_state);
                    break;

                // lp_mode
                case 2:
                    if (VirtualBtn == BTN_DOWN)
                        vtx_state = 3;
                    else if (VirtualBtn == BTN_UP)
                        vtx_state = 1;
                    else if (VirtualBtn == BTN_LEFT) {
                        if (SA_lock == 0) {
                            vtx_lp--;
                            if (vtx_lp > 2)
                                vtx_lp = 2;
                        }
                    } else if (VirtualBtn == BTN_RIGHT) {
                        if (SA_lock == 0) {
                            vtx_lp++;
                            if (vtx_lp > 2)
                                vtx_lp = 0;
                        }
                    }
                    update_vtx_menu_param(vtx_state);
                    break;

                // pit_mode
                case 3:
                    if (VirtualBtn == BTN_DOWN)
                        vtx_state = 4;
                    else if (VirtualBtn == BTN_UP)
                        vtx_state = 2;
                    else if (VirtualBtn == BTN_RIGHT) {
                        if (SA_lock == 0) {
                            if (vtx_pit == PIT_0MW)
                                vtx_pit = PIT_OFF;
                            else
                                vtx_pit++;
                        }
                    } else if (VirtualBtn == BTN_LEFT) {
                        if (SA_lock == 0) {
                            if (vtx_pit == PIT_OFF)
                                vtx_pit = PIT_0MW;
                            else
                                vtx_pit--;
                        }
                    }
                    update_vtx_menu_param(vtx_state);
                    break;

                // offset_25mw
                case 4:
                    if (VirtualBtn == BTN_DOWN)
                        vtx_state = 5;
                    else if (VirtualBtn == BTN_UP)
                        vtx_state = 3;
                    else if (VirtualBtn == BTN_RIGHT) {
                        if (vtx_offset == 10)
                            vtx_offset = vtx_offset;
                        else if (vtx_offset == 11)
                            vtx_offset = 0;
                        else if (vtx_offset < 10)
                            vtx_offset++;
                        else
                            vtx_offset--;
                    } else if (VirtualBtn == BTN_LEFT) {
                        if (vtx_offset == 20)
                            vtx_offset = vtx_offset;
                        else if (vtx_offset == 0)
                            vtx_offset = 11;
                        else if (vtx_offset > 10)
                            vtx_offset++;
                        else
                            vtx_offset--;
                    }
                    update_vtx_menu_param(vtx_state);
                    break;

                // exit
                case 5:
                    if (VirtualBtn == BTN_DOWN) {
                        vtx_state = 6;
                        update_vtx_menu_param(vtx_state);
                    } else if (VirtualBtn == BTN_UP) {
                        vtx_state = 4;
                        update_vtx_menu_param(vtx_state);
                    } else if (VirtualBtn == BTN_RIGHT) {
                        vtx_state = 0;
                        cms_state = CMS_OSD;
                        fc_init();
                        msp_tx_cnt = 0;
                    }
                    break;

                // save&exit
                case 6:
                    if (VirtualBtn == BTN_DOWN) {
                        vtx_state = 0;
                        update_vtx_menu_param(vtx_state);
                    } else if (VirtualBtn == BTN_UP) {
                        vtx_state = 5;
                        update_vtx_menu_param(vtx_state);
                    } else if (VirtualBtn == BTN_RIGHT) {
                        vtx_state = 0;
                        cms_state = CMS_OSD;
                        if (SA_lock) {
                            RF_FREQ = vtx_channel;
                            RF_POWER = vtx_power;
                            LP_MODE = vtx_lp;
                            PIT_MODE = vtx_pit;
                            vtx_pit_save = vtx_pit;
                            OFFSET_25MW = vtx_offset;
                            CFG_Back();
                            Setting_Save();
                        } else {
                            save_vtx_param();
                        }
                        fc_init();
                        msp_tx_cnt = 0;
                    }
                    break;
                default:
                    cms_state = CMS_OSD;
                    fc_init();
                    break;
                } // switch
            }     // if(last_mid)
            // last_mid = mid;
        } else {
            cms_state = CMS_OSD;
            fc_init();
        }
        break;
    }

    case CMS_ENTER_CAM: {
        if (VirtualBtn == BTN_ENTER)
            cms_cnt++;
        else
            cms_state = CMS_OSD;

        if (cms_cnt == 5) {
            cms_cnt = 0;
            disp_mode = DISPLAY_CMS;
            clear_screen();
            camMenuInit();
            cms_state = CMS_CAM;
        }
        break;
    }

    case CMS_CAM: {
        // detect to exit
        if (VirtualBtn == BTN_EXIT)
            cms_cnt++;
        else
            cms_cnt = 0;

        if (camStatusUpdate(VirtualBtn) || (cms_cnt == 10)) {
            disp_mode = DISPLAY_OSD;
            cms_state = CMS_OSD;
            fc_init();
            msp_tx_cnt = 0;
        }

        break;
    }

    default: {
        cms_state = CMS_OSD;
        fc_init();
        break;
    }
    } // switch
    last_mid = mid;
}

void vtx_menu_init() {
    uint8_t i;
    uint8_t offset = (resolution == HD_5018) ? 8 : 0;
    uint8_t hourString[4];
    uint8_t minuteString[2];

    disp_mode = DISPLAY_CMS;
    clear_screen();

    strcpy(osd_buf[0] + offset + 2, "----VTX_MENU----");
    strcpy(osd_buf[2] + offset + 2, ">CHANNEL");
    strcpy(osd_buf[3] + offset + 3, "POWER");
    strcpy(osd_buf[4] + offset + 3, "LP_MODE");
    strcpy(osd_buf[5] + offset + 3, "PIT_MODE");
    strcpy(osd_buf[6] + offset + 3, "OFFSET_25MW");
    strcpy(osd_buf[7] + offset + 3, "EXIT");
    strcpy(osd_buf[8] + offset + 3, "SAVE&EXIT");
    strcpy(osd_buf[9] + offset + 2, "------INFO------");
    strcpy(osd_buf[10] + offset + 3, "VTX");
    strcpy(osd_buf[11] + offset + 3, "VER");
    strcpy(osd_buf[12] + offset + 3, "LIFETIME");

    for (i = 0; i < 5; i++) {
        osd_buf[2 + i][offset + 19] = '<';
        osd_buf[2 + i][offset + 26] = '>';
    }

// draw variant
#if defined VTX_B
    strcpy(osd_buf[10] + offset + 10, "            BF");
#elif defined VTX_M
    strcpy(osd_buf[10] + offset + 10, "FREESTYLE LITE");
#elif defined VTX_S
    strcpy(osd_buf[10] + offset + 10, "         WHOOP");
#elif defined VTX_R
    strcpy(osd_buf[10] + offset + 10, "          RACE");
#elif defined VTX_WL
    strcpy(osd_buf[10] + offset + 10, "    WHOOP LITE");
#elif defined VTX_L
    strcpy(osd_buf[10] + offset + 10, "     FREESTYLE");
#endif
    // draw version
    osd_buf[11][offset + 22] = (uint8_t)((VERSION >> 4) & 0x0f) + '0';
    osd_buf[11][offset + 23] = (uint8_t)(VERSION & 0x0f) + '0';
#ifdef BETA
    osd_buf[11][offset + 25] = 'B';
    osd_buf[11][offset + 26] = (uint8_t)((BETA >> 4) & 0x0f) + '0';
    osd_buf[11][offset + 27] = (uint8_t)(BETA & 0x0f) + '0';
#endif

    ParseLifeTime(hourString, minuteString);
    osd_buf[12][offset + 16] = hourString[0];
    osd_buf[12][offset + 17] = hourString[1];
    osd_buf[12][offset + 18] = hourString[2];
    osd_buf[12][offset + 19] = hourString[3];
    osd_buf[12][offset + 20] = 'H';
    osd_buf[12][offset + 21] = minuteString[0];
    osd_buf[12][offset + 22] = minuteString[1];
    osd_buf[12][offset + 23] = 'M';

    vtx_channel = RF_FREQ;
    vtx_power = RF_POWER;
    vtx_lp = LP_MODE;
    vtx_pit = PIT_MODE;
    vtx_offset = OFFSET_25MW;
    update_vtx_menu_param(0);
}

void update_vtx_menu_param(uint8_t vtx_state) {
    uint8_t i;
    uint8_t offset = (resolution == HD_5018) ? 8 : 0;
    uint8_t hourString[4];
    uint8_t minuteString[2];

    // state
    for (i = 0; i < 7; i++) {
        if (i == vtx_state)
            osd_buf[i + 2][offset + 2] = '>';
        else
            osd_buf[i + 2][offset + 2] = ' ';
    }

    // channel display
    if (vtx_channel < 8) {
        osd_buf[2][offset + 23] = 'R';
        osd_buf[2][offset + 24] = vtx_channel + '1';
    } else {
        osd_buf[2][offset + 23] = 'F';
        if (vtx_channel == 8)
            osd_buf[2][offset + 24] = '2';
        else if (vtx_channel == 9)
            osd_buf[2][offset + 24] = '4';
    }

    // power display
    switch (vtx_power) {
    case 0:
        strcpy(osd_buf[3] + offset + 20, "   25");
        break;
    case 1:
        strcpy(osd_buf[3] + offset + 20, "  200");
        break;
    case 2:
#ifdef VTX_B
        strcpy(osd_buf[3] + offset + 20, "  450");
#else
        strcpy(osd_buf[3] + offset + 20, "  500");
#endif
        break;
    case 3:
        strcpy(osd_buf[3] + offset + 20, "  MAX");
        break;
    default:
        strcpy(osd_buf[3] + offset + 20, "     ");
        break;
    }

    if (vtx_lp == 0)
        strcpy(osd_buf[4] + offset + 20, "  OFF");
    else if (vtx_lp == 1)
        strcpy(osd_buf[4] + offset + 20, "   ON");
    else if (vtx_lp == 2)
        strcpy(osd_buf[4] + offset + 20, "  1ST");

    if (vtx_pit == PIT_P1MW)
        strcpy(osd_buf[5] + offset + 20, " P1MW");
    else if (vtx_pit == PIT_0MW)
        strcpy(osd_buf[5] + offset + 20, "  0MW");
    else if (vtx_pit == PIT_OFF)
        strcpy(osd_buf[5] + offset + 20, "  OFF");

    if (vtx_offset < 10) {
        strcpy(osd_buf[6] + offset + 20, "     ");
        osd_buf[6][offset + 23] = '0' + vtx_offset;
    } else if (vtx_offset == 10)
        strcpy(osd_buf[6] + offset + 20, "   10");
    else if (vtx_offset < 20) {
        strcpy(osd_buf[6] + offset + 20, "   -");
        osd_buf[6][offset + 24] = '0' + (vtx_offset - 10);
    } else if (vtx_offset == 20)
        strcpy(osd_buf[6] + offset + 20, "  -10");

    ParseLifeTime(hourString, minuteString);
    osd_buf[12][offset + 16] = hourString[0];
    osd_buf[12][offset + 17] = hourString[1];
    osd_buf[12][offset + 18] = hourString[2];
    osd_buf[12][offset + 19] = hourString[3];
    osd_buf[12][offset + 20] = 'H';
    osd_buf[12][offset + 21] = minuteString[0];
    osd_buf[12][offset + 22] = minuteString[1];
    osd_buf[12][offset + 23] = 'M';
}

void save_vtx_param() {
    RF_FREQ = vtx_channel;
    RF_POWER = vtx_power;
    LP_MODE = vtx_lp;
    PIT_MODE = vtx_pit;
    vtx_pit_save = vtx_pit;
    OFFSET_25MW = vtx_offset;
    CFG_Back();
    Setting_Save();
    Imp_RF_Param();

    // init pitmode status and first_arm after setting_save
    pit_mode_cfg_done = 0;
    lp_mode_cfg_done = 0;
    first_arm = 0;

    if (!SA_lock) {
        if (vtx_pit == PIT_0MW)
            msp_set_vtx_config(POWER_MAX + 1, 0);
        else
            msp_set_vtx_config(RF_POWER, 1);
    }
}

void set_vtx_param() {
    // If fc is lost, auto armed
    /*
    if(seconds >= PWR_LMT_SEC){
        if(!fc_lock)
            g_IS_ARMED = 1;
    }
    */
    if (SA_lock)
        return;

    if (!g_IS_ARMED) {
        // configurate pitmode when power-up or setting_vtx
        if (PIT_MODE) {
            if (!pit_mode_cfg_done) {
                if (vtx_pit_save == PIT_0MW) {
                    WriteReg(0, 0x8F, 0x10);
// SPI_Write(0x6, 0xFF0, 0x00000018);
// SPI_Write(0x3, 0xd00, 0x00000000);
#ifdef _DEBUG_MODE
                    debugf("\r\nDM6300 0mW");
#endif
                    cur_pwr = POWER_MAX + 2;
                    temp_err = 1;
                } else // if(vtx_pit_save == PIT_P1MW)
                {
                    DM6300_SetPower(POWER_MAX + 1, RF_FREQ, 0);
#ifdef _DEBUG_MODE
                    debugf("\r\nDM6300 P1mW");
#endif
                    cur_pwr = POWER_MAX + 1;
                }
                pit_mode_cfg_done = 1;
            }
        } else if (LP_MODE) {
            if (!lp_mode_cfg_done) {
                DM6300_SetPower(0, RF_FREQ, 0); // limit power to 25mW
                cur_pwr = 0;
#ifdef _DEBUG_MODE
                debugf("\n\rEnter LP_MODE");
#endif
                lp_mode_cfg_done = 1;
            }
        }
    }

    if (g_IS_ARMED && !g_IS_ARMED_last) {
        // Power_Auto
        if (vtx_pit_save == PIT_0MW) {
            // exit 0mW
            Init_6300RF(RF_FREQ, RF_POWER);
            DM6300_AUXADC_Calib();
            cur_pwr = RF_POWER;
            vtx_pit = PIT_OFF;
            vtx_pit_save = PIT_OFF;
        } else if (PIT_MODE || LP_MODE) {
// exit pitmode or lp_mode
#ifdef _DEBUG_MDOE
            debugf("\n\rExit PIT or LP");
#endif
#ifndef VIDEO_PAT
#ifdef VTX_L
            if (RF_POWER == 3 && !g_IS_ARMED)
                pwr_lmt_done = 0;
            else
#endif
#endif
            {
                DM6300_SetPower(RF_POWER, RF_FREQ, pwr_offset);
                cur_pwr = RF_POWER;
            }
        }

        first_arm = 1;
        PIT_MODE = PIT_OFF;
        Setting_Save();
        msp_set_vtx_config(RF_POWER, 1);
    } else if (!g_IS_ARMED && g_IS_ARMED_last) {
        if (LP_MODE == 1) {
            DM6300_SetPower(0, RF_FREQ, 0); // limit power to 25mW during disarmed
            cur_pwr = 0;
#ifdef _DEBUG_MDOE
            debugf("\n\rEnter LP_MODE");
#endif
        }
    }

    g_IS_ARMED_last = g_IS_ARMED;
}

#ifdef INIT_VTX_TABLE
const uint8_t bf_vtx_band_table[6][31] = {
    /*BOSCAM_A*/
    {/*0x24,0x4d,0x3c,*/ 0x1d, 0xe3, 0x01, 0x08, 'B', 'O', 'S', 'C', 'A', 'M', '_', 'A', 'A', 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /*BOSCAM_B*/
    {/*0x24,0x4d,0x3c,*/ 0x1d, 0xe3, 0x02, 0x08, 'B', 'O', 'S', 'C', 'A', 'M', '_', 'B', 'B', 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /*BOSCAM_R*/
    {/*0x24,0x4d,0x3c,*/ 0x1d, 0xe3, 0x03, 0x08, 'B', 'O', 'S', 'C', 'A', 'M', '_', 'E', 'E', 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /*FATSHARK*/
    {/*0x24,0x4d,0x3c,*/ 0x1d, 0xe3, 0x04, 0x08, 'F', 'A', 'T', 'S', 'H', 'A', 'R', 'K', 'F', 0x01, 0x08, 0x00, 0x00, 0x80, 0x16, 0x00, 0x00, 0xa8, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /*RACEBAND*/
    {/*0x24,0x4d,0x3c,*/ 0x1d, 0xe3, 0x05, 0x08, 'R', 'A', 'C', 'E', 'B', 'A', 'N', 'D', 'R', 0x01, 0x08, 0x1a, 0x16, 0x3f, 0x16, 0x64, 0x16, 0x89, 0x16, 0xae, 0x16, 0xd3, 0x16, 0xf8, 0x16, 0x1d, 0x17},
    /*IMD6*/
    {/*0x24,0x4d,0x3c,*/ 0x1d, 0xe3, 0x06, 0x08, 'I', 'M', 'D', '6', ' ', ' ', ' ', ' ', 'I', 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};
const uint8_t bf_vtx_power_table[5][9] = {
    {/*0x24,0x4d,0x3c,*/ 0x07, 0xe4, 0x01, 0x0e, 0x00, 0x03, '2', '5', ' '}, // 25mW
    {/*0x24,0x4d,0x3c,*/ 0x07, 0xe4, 0x02, 0x17, 0x00, 0x03, '2', '0', '0'}, // 200mW
    {/*0x24,0x4d,0x3c,*/ 0x07, 0xe4, 0x03, 0x00, 0x00, 0x03, '0', ' ', ' '}, // 0mW
    {/*0x24,0x4d,0x3c,*/ 0x07, 0xe4, 0x04, 0x00, 0x00, 0x03, '0', ' ', ' '}, // 0mW
    {/*0x24,0x4d,0x3c,*/ 0x07, 0xe4, 0x05, 0x00, 0x00, 0x03, '0', ' ', ' '}, // 0mW
};
const uint8_t bf_vtx_power_500mW[9] = {0x07, 0xe4, 0x03, 0x1b, 0x00, 0x03, '5', '0', '0'}; // 500mW
const uint8_t bf_vtx_power_1W[9] = {0x07, 0xe4, 0x04, 0x1e, 0x00, 0x03, 'M', 'A', 'X'};    // MAX

void InitVtxTable() {
    uint8_t i, j;
    uint8_t crc;
    uint8_t const *power_table[5];

#ifdef _DEBUG_MODE
    debugf("\r\nInitVtxTable");
#endif

    // set band num, channel num and power level number
    LP_MODE = fc_lp_rx;
    msp_set_vtx_config(fc_pwr_rx, 0);

    // set band/channel
    for (i = 0; i < 6; i++) {
        msp_send_header(1);
        crc = 0;
        for (j = 0; j < 31; j++) {
            CMS_tx(bf_vtx_band_table[i][j]);
            crc ^= bf_vtx_band_table[i][j];
        }
        CMS_tx(crc);
    }

    // first two entries are always the same
    power_table[0] = bf_vtx_power_table[0];
    power_table[1] = bf_vtx_power_table[1];

#if defined VTX_L
    if (!powerLock) {
        // if we dont have power lock, enable 500mw and 1W
        power_table[2] = bf_vtx_power_500mW;
        power_table[3] = bf_vtx_power_1W;
    } else {
        // otherwise add empties
        power_table[2] = bf_vtx_power_table[2];
        power_table[3] = bf_vtx_power_table[3];
    }
#elif defined VTX_M
    power_table[2] = bf_vtx_power_500mW;
    power_table[3] = bf_vtx_power_table[3];
#else
    power_table[2] = bf_vtx_power_table[2];
    power_table[3] = bf_vtx_power_table[3];
#endif

    // last entry is alway the same
    power_table[4] = bf_vtx_power_table[4];

    // send all of them
    for (i = 0; i < 5; i++) {
        msp_send_header(1);
        crc = 0;
        for (j = 0; j < 9; j++) {
            CMS_tx(bf_vtx_power_table[i][j]);
            crc ^= bf_vtx_power_table[i][j];
        }
        CMS_tx(crc);
    }

    msp_eeprom_write();
}
#endif

#else

void fc_init() {}
void msp_task() {}
void msp_set_vtx_config(uint8_t power, uint8_t save) {}

#endif