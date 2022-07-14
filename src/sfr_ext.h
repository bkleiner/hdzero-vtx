#ifndef __SFR_EXT_H_
#define __SFR_EXT_H_

__sfr __at (0xB9) SFR_CMD;
__sfr __at (0xBA) SFR_DATA;
__sfr __at (0xBB) SFR_ADDRL;
__sfr __at (0xBC) SFR_ADDRH;
__sfr __at (0xBD) SFR_BUSY;
__sfr __at (0xFB) DBG_PIN0;
__sfr __at(0xC0) SCON1;
__sfr __at(0xC1) SBUF1;

void WriteReg(uint8_t page, uint8_t addr, uint8_t dat);
uint8_t ReadReg(uint8_t page, uint8_t addr);

void Write936x(uint16_t addr, uint8_t dat);
uint8_t Read936x(uint16_t addr);

/*
void OSD_Mark_wr(uint16_t addr, uint8_t dat);
void OSD_Map_wr(uint16_t addr, uint8_t dat);
void OSD_CH_wr(uint16_t addr, uint8_t dat);
*/
void DP_tx(uint8_t dat);

#endif
