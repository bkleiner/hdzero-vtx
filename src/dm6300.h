#ifndef __DM6300_H_
#define __DM6300_H_

#include "stdint.h"

#define EFUSE_NUM 12   // 12 macro
#define EFUSE_SIZE 128 // 128*8=1024bit

typedef enum {
    DM6300_BW_27M,
    DM6300_BW_20M,
} dm6300_bw_t;

typedef struct {
    uint8_t chip_name[8];
    uint8_t chip_ver[4];
    uint8_t chip_id[8];
    uint8_t chip_grade[4];
    uint8_t crc[4];
    uint8_t efuse_ver[4];
    uint8_t rsvd1[32];
    uint16_t mode;
    uint16_t band_num;
    uint8_t rsvd2[60];
} dm6300_macro0_t;

typedef struct {
    uint16_t ical;
    uint16_t rcal;
    uint32_t bandgap;
    uint32_t tempsensor;
    uint8_t rsvd[116];
} dm6300_macro1_t;

typedef struct {
    uint16_t freq_start;
    uint16_t freq_stop;
    uint32_t iqmismatch[3];
    uint32_t im2;
    uint8_t rsvd[12];
} dm6300_rx_cal_t;

typedef struct {
    uint16_t freq_start;
    uint16_t freq_stop;
    uint32_t dcoc_i;
    uint32_t dcoc_q;
    uint32_t iqmismatch;
    uint32_t dcoc_i_dvm;
    uint32_t dcoc_q_dvm;
    uint8_t rsvd[8];
} dm6300_tx_cal_t;

typedef struct {
    dm6300_rx_cal_t rx1;
    dm6300_tx_cal_t tx1;
    dm6300_rx_cal_t rx2;
    dm6300_tx_cal_t tx2;
} dm6300_macro2_t;

typedef struct {
    dm6300_macro0_t m0;
    dm6300_macro1_t m1;
    dm6300_macro2_t m2[EFUSE_NUM - 2];
} dm6300_macros_t;

typedef union {
    uint8_t raw[EFUSE_NUM][EFUSE_SIZE];
    dm6300_macros_t macros;
} dm6300_efuse_t;

void dm6300_init(uint8_t ch);
void dm6300_set_channel(uint8_t ch);
void dm6300_set_power(uint8_t pwr, uint8_t ch, uint8_t offset);

#endif /* __DM6300_H_ */
