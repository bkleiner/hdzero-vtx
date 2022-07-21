#ifndef __OSD_DISPLAY_H_
#define __OSD_DISPLAY_H_

#include "stdint.h"

#include "toolchain.h"

#define SD_HMAX 30
#define SD_VMAX 16
#define HD_HMAX 50
#define HD_VMAX 18

typedef enum {
    DISPLAY_OSD,
    DISPLAY_CMS,
} osd_display_mode_t;

typedef enum {
    SD_3016,
    HD_5018,
    HD_3016,
} osd_resolution_t;

void osd_init();
void osd_task();

// resets the osd to its inital state
// and clears the screen
void osd_reset();

void osd_set_config(uint8_t font, osd_resolution_t res);

void osd_clear_screen();
void osd_write_data(uint8_t row, uint8_t col, uint8_t page, uint8_t *data, uint8_t len);

// must be called to submit the current framebuffer for sending
void osd_submit();

// TODO: eliminate
extern uint8_t osd_buf[HD_VMAX][HD_HMAX];
extern uint8_t osd_mode;
extern uint8_t resolution;

#endif /* __OSD_DISPLAY_H_ */
