#ifndef __VTX_H_
#define __VTX_H_

#include "stdint.h"

#include "camera.h"

typedef struct {
    camera_mode_t cam_mode;

    uint8_t msp_variant[4];

    uint8_t font_type;
} vtx_state_t;

extern vtx_state_t vtx_state;

void vtx_pattern_init();
void vtx_rf_init(uint8_t channel, uint8_t power);

#endif /* __VTX_H_ */
