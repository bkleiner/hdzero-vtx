#include "msp.h"

#include "driver/uart.h"

#include "debug.h"
#include "osd.h"
#include "util.h"

typedef enum {
    SUBCMD_HEARTBEAT,
    SUBCMD_RELEASE,
    SUBCMD_CLEAR,
    SUBCMD_WRITE,
    SUBCMD_DRAW,
    SUBCMD_CONFIG,
} msp_displayport_subcmd_t;

typedef enum {
    STATE_START,
    STATE_HEADER,
    STATE_DIRECTION,

    STATE_V1_LENGTH,
    STATE_V1_CMD,
    STATE_V1_PAYLOAD,

    STATE_V2_FLAG,
    STATE_V2_CMD_LOW,
    STATE_V2_CMD_HIGH,
    STATE_V2_LENGTH_LOW,
    STATE_V2_LENGTH_HIGH,
    STATE_V2_PAYLOAD,

    STATE_CRC,

} msp_parser_state_t;

uint8_t msp_payload[0xFF];

void msp_init() {
}

void msp_handle_displayport(const uint8_t *payload, const uint8_t length) {
    switch (payload[0]) {
    case SUBCMD_CLEAR:
        osd_clear();
        break;

    case SUBCMD_WRITE: {
        osd_write_data(payload[1], payload[2], payload[3] & 0x01, payload + 4, length - 4);
        break;
    }

    case SUBCMD_DRAW:
        osd_submit();
        break;

    case SUBCMD_CONFIG:
        osd_set_config(payload[1], payload[2]);
        break;

    default:
    case SUBCMD_HEARTBEAT:
    case SUBCMD_RELEASE:
        break;
    }
}

void msp_handle_cmd(const uint16_t cmd, const uint8_t *payload, const uint16_t length) {
    switch (cmd) {
    case MSP_CMD_DISPLAYPORT:
        msp_handle_displayport(payload, length);
        break;

    default:
        debugf("msp: unknown cmd %d\r\n", cmd);
        break;
    }
}

void msp_task() {
    static msp_parser_state_t state = STATE_START;

    static uint8_t magic = 0;

    static uint16_t length = 0;
    static uint16_t cmd = 0;
    static uint8_t crc = 0;

    static uint8_t offset = 0;

    uint8_t data = 0;
    while (1) {
        if (!msp_uart_read(&data)) {
            break;
        }

        switch (state) {
        case STATE_START:
            if (data == MSP_START_BYTE) {
                state = STATE_HEADER;
            }
            break;

        case STATE_HEADER:
            if (data == MSP1_MAGIC || data == MSP2_MAGIC) {
                magic = data;
                state = STATE_DIRECTION;
            } else {
                state = STATE_START;
            }
            break;

        case STATE_DIRECTION:
            if (data == '>') {
                if (magic == MSP1_MAGIC) {
                    state = STATE_V1_LENGTH;
                } else {
                    state = STATE_V2_FLAG;
                }
            } else {
                state = STATE_START;
            }
            break;

        case STATE_V1_LENGTH:
            crc = data;
            length = data;
            state = STATE_V1_CMD;
            break;

        case STATE_V1_CMD:
            crc = crc ^ data;
            cmd = data;
            offset = 0;
            if (length == 0) {
                state = STATE_CRC;
            } else {
                state = STATE_V1_PAYLOAD;
            }
            break;

        case STATE_V1_PAYLOAD:
            msp_payload[offset++] = data;
            crc = crc ^ data;
            if (offset == length) {
                state = STATE_CRC;
            }
            break;

        case STATE_V2_FLAG:
            if (data == 0x0) {
                crc = crc8tab[data];
                state = STATE_V2_CMD_LOW;
            } else {
                state = STATE_START;
            }
            break;

        case STATE_V2_CMD_LOW:
            cmd = data;
            crc = crc8tab[crc ^ data];
            state = STATE_V2_CMD_HIGH;
            break;

        case STATE_V2_CMD_HIGH:
            cmd = (uint16_t)data << 8;
            crc = crc8tab[crc ^ data];
            state = STATE_V2_LENGTH_LOW;
            break;

        case STATE_V2_LENGTH_LOW:
            length = data;
            crc = crc8tab[crc ^ data];
            state = STATE_V2_LENGTH_HIGH;
            break;

        case STATE_V2_LENGTH_HIGH:
            length = (uint16_t)data << 8;
            crc = crc8tab[crc ^ data];
            offset = 0;
            if (length == 0) {
                state = STATE_CRC;
            } else {
                state = STATE_V2_PAYLOAD;
            }
            break;

        case STATE_V2_PAYLOAD:
            msp_payload[offset++] = data;
            crc = crc8tab[crc ^ data];
            if (offset == length) {
                state = STATE_CRC;
            }
            break;

        case STATE_CRC:
            if (data == crc) {
                msp_handle_cmd(cmd, msp_payload, length);
            }
            state = STATE_START;
            break;
        }
    }
}