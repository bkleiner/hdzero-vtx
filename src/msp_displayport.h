#ifndef __MSP_DISPLAYPORT_H_
#define __MSP_DISPLAYPORT_H_

#include "common.h"

#define IS_HI(x)  ((x) > 1750)
#define IS_LO(x)  ((x) < 1250)
#define IS_MID(x) ((!IS_HI(x)) && (!IS_LO(x)))

#define SD_HMAX 30
#define SD_VMAX 16
#define HD_HMAX 50
#define HD_VMAX 18

#define TXBUF_SIZE 69

#define MSP_HEADER_START_BYTE   0x24
#define MSP_HEADER_M_BYTE       0x4D
#define MSP_HEADER_M2_BYTE      0x58
#define MSP_PACKAGE_REPLAY_BYTE 0x3E

#define MSP_CMD_FC_VARIANT       0x02
#define MSP_CMD_VTX_CONFIG       0x58
#define MSP_CMD_STATUS_BYTE      0x65
#define MSP_CMD_RC_BYTE          0x69
#define MSP_CMD_DISPLAYPORT_BYTE 0xB6

#define DP_HEADER0 0x56
#define DP_HEADER1 0x80

#define FC_OSD_LOCK            0x01
#define FC_VARIANT_LOCK        0x02
#define FC_RC_LOCK             0x04
#define FC_VTX_CONFIG_LOCK     0x08
#define FC_STATUS_LOCK         0x10
#define FC_INIT_VTX_TABLE_LOCK 0x80

typedef enum {
    BTN_UP,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_MID,
    BTN_ENTER,
    BTN_EXIT,
    BTN_INVALID
} ButtonEvent_e;

typedef enum {
    BTFL,
    INAV,
    ARDU
} fc_variant_e;

typedef enum {
    PIT_OFF,
    PIT_P1MW,
    PIT_0MW
} vtxPtiType_e;

typedef enum {
    MSP_HEADER_START,
    MSP_HEADER_M,
    MSP_PACKAGE_REPLAY1,
    MSP_LENGTH,
    MSP_CMD,
    MSP_RX1,
    MSP_CRC1,
    MSP_HEADER_M2,
    MSP_PACKAGE_REPLAY2,
    MSP_ZERO,
    MSP_CMD_L,
    MSP_CMD_H,
    MSP_LEN_L,
    MSP_LEN_H,
    MSP_RX2,
    MSP_CRC2,

} msp_rx_status_e;

typedef enum {
    // msp_displayport
    MSP_OSD_SUBCMD,
    MSP_OSD_LOC,
    MSP_OSD_ATTR,
    MSP_OSD_WRITE,
    MSP_OSD_CONFIG,
} msp_osd_status_e;

typedef enum {
    CUR_DISPLAYPORT,
    CUR_RC,
    CUR_STATUS,
    CUR_FC_VARIANT,
    CUR_VTX_CONFIG,
    CUR_OTHERS
} cur_cmd_e;

typedef enum {
    SUBCMD_HEARTBEAT,
    SUBCMD_RELEASE,
    SUBCMD_CLEAR,
    SUBCMD_WRITE,
    SUBCMD_DRAW,
    SUBCMD_CONFIG,
} displayport_subcmd_e;

typedef enum {
    CMS_OSD,
    CMS_ENTER_0MW,
    CMS_EXIT_0MW,
    CMS_ENTER_VTX_MENU,
    CMS_CONFIG_NORMAL_POWER,
    CMS_ENTER_CAM,
    CMS_VTX_MENU,
    CMS_CAM
} cms_state_e;

void msp_init();
void msp_task();
uint8_t msp_read_one_frame();
void msp_cmd_tx();
void parse_status();
void parse_rc();
void parse_variant();
void parse_vtx_config();
uint8_t parse_displayport(uint8_t len);

void update_cms_menu(uint16_t roll, uint16_t pitch, uint16_t yaw, uint16_t throttle);
void vtx_menu_init();
void update_vtx_menu_param(uint8_t vtx_state);
void save_vtx_param();
void msp_set_vtx_config(uint8_t power, uint8_t save);
void set_vtx_param();
uint8_t msp_cmp_fc_variant(const char *variant);

#ifdef INIT_VTX_TABLE
void InitVtxTable();
#endif

extern uint8_t osd_menu_offset;
extern uint8_t msp_tx_cnt;
extern uint8_t mspVtxLock;
#endif /* __MSP_DISPLAYPORT_H_ */
