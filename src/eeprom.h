#ifndef __EEPROM_H_
#define __EEPROM_H_

#include "stdint.h"

#define EEPROM_SIZE 0xFF
#define EEPROM_ADDR_MAGIC 0xFA

#define CAMERA_WBMODE_MAX 4

typedef struct {
    uint8_t brightness;
    uint8_t sharpness;
    uint8_t saturation;
    uint8_t contrast;
    uint8_t hv_flip;
    uint8_t night_mode;
    uint8_t ratio;
    uint8_t wb_mode;
    uint8_t wb_red[CAMERA_WBMODE_MAX];
    uint8_t wb_blue[CAMERA_WBMODE_MAX];
} eeprom_camera_profile_t;

typedef struct {
    uint8_t frequency;
    uint8_t power;
    uint8_t lp_mode;
    uint8_t pit_mode;
    uint8_t offset_25mw;
    uint8_t _unused0[4];
    uint8_t sa_lock;
    uint8_t power_lock;
    uint8_t _unused1;
} eeprom_vtx_config_t;

typedef struct {
    uint8_t enable;
    uint8_t ih;
    uint8_t il;
    uint8_t qh;
    uint8_t ql;
} eeprom_dcoc_param_t;

typedef struct {
    uint8_t power_table[0x40];                  // 0x00 - 0x40
    eeprom_camera_profile_t camera_profiles[4]; // 0x40 - 0x80
    eeprom_vtx_config_t vtx_config;             // 0x80 - 0x8c
    uint8_t _unused0[0x34];                     // 0x8C - 0xC0
    eeprom_dcoc_param_t dcoc;                   // 0xC0 - 0xC5
    uint8_t _unused1[0x2B];                     // 0xC5 - 0xF0
    uint8_t lifetime[4];                        // 0xF0 - 0xF5
    uint8_t _unused2[6];                        // 0xF5 - 0xFA
    uint8_t magic;                              // 0xFA - 0xFB
    uint8_t _unused3[4];                        // 0xFB - 0xFF
} eeprom_storage_t;

extern eeprom_storage_t *storage;

void eeprom_init();
void eeprom_load();
void eeprom_save();

#endif /* __EEPROM_H_ */
