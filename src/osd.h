#ifndef __OSD_H_
#define __OSD_H_

#include "stdint.h"

#include "vtx.h"

#define SD_HMAX 30
#define SD_VMAX 16

#define HD_HMAX 50
#define HD_VMAX 18

void osd_init();
void osd_task();

void osd_clear();
void osd_set_config(uint8_t font, osd_resolution_t res);
void osd_write_data(uint8_t row, uint8_t col, uint8_t page_extend, const uint8_t *data, uint8_t len);
void osd_submit();

#endif /* __OSD_H_ */
