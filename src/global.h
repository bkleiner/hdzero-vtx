#ifndef __GLOBAL_H_
#define __GLOBAL_H_

#define HI_BYTE(a) ((a >> 8) & 0xFF)
#define LO_BYTE(a) (a & 0xFF)

#define ASSERT(a)
#define abs(a) (((a) < 0) ? (-(a)) : (a))

#define SET_ASCII(res, val) \
    temp = (val);           \
    res = '0' + temp;

#define SET_ASCII_NON_ZERO(res, val) \
    temp = (val);                    \
    if (temp == 0) {                 \
        res = ' ';                   \
    } else {                         \
        res = '0' + temp;            \
    }

uint8_t Asc2Bin(uint8_t *s);
uint16_t Asc4Bin(uint8_t *s);
uint32_t Asc8Bin(uint8_t *s);

uint8_t stricmp(uint8_t *ptr1, uint8_t *ptr2);

void uint8ToString(uint8_t dec, uint8_t *Str);

#ifdef SDCC
void WAIT(uint16_t ms);
#else
void WAIT(uint32_t ms);
#endif

extern uint8_t crc8tab[256];

#endif /* __GLOBAL_H_ */
