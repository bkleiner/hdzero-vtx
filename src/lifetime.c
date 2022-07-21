#include "lifetime.h"

#include "debug.h"

uint32_t sysLifeTime = 0;
uint32_t sysLifeTime_last = 0;
uint8_t LifeTimeOP = 0;
void Get_EEP_LifeTime(void) {
    uint8_t u8;

    u8 = I2C_Read8(ADDR_EEPROM, EEP_ADDR_LIFETIME_0);
    sysLifeTime = (uint32_t)u8;
    u8 = I2C_Read8(ADDR_EEPROM, EEP_ADDR_LIFETIME_1);
    sysLifeTime += (uint32_t)u8 << 8;
    u8 = I2C_Read8(ADDR_EEPROM, EEP_ADDR_LIFETIME_2);
    sysLifeTime += (uint32_t)u8 << 16;
    u8 = I2C_Read8(ADDR_EEPROM, EEP_ADDR_LIFETIME_3);
    sysLifeTime += (uint32_t)u8 << 24;

    if (sysLifeTime == 0xffffffff) {
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_0, 0x00);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_1, 0x00);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_2, 0x00);
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_3, 0x00);
        sysLifeTime = 0;
    }

    sysLifeTime_last = sysLifeTime;
}
void Update_EEP_LifeTime(void) {
    uint8_t u8;
    uint32_t diff;
    static uint16_t lstSeconds = 0;

#ifdef _DEBUG_MODE
    return;
#endif
#ifdef VIDEO_PAT
    return;
#endif
#ifdef _RF_CALIB
    return;
#endif

    if (seconds - lstSeconds >= 10) {
        sysLifeTime++;
        lstSeconds = seconds;
        if (sysLifeTime >= 3599999)
            sysLifeTime = 3599999;
    } else {
        return;
    }

    diff = sysLifeTime_last ^ sysLifeTime;

#ifdef _DEBUG_LIFETIME
    debugf("\r\nsysLifeTime:%ld", sysLifeTime);
#endif

    if ((diff >> 0) & 0xff) {
        u8 = (sysLifeTime >> 0) & 0xff;
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_0, u8);
    }

    if ((diff >> 8) & 0xff) {
        u8 = (sysLifeTime >> 8) & 0xff;
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_1, u8);
    }

    if ((diff >> 16) & 0xff) {
        u8 = (sysLifeTime >> 16) & 0xff;
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_2, u8);
    }

    if ((diff >> 24) & 0xff) {
        u8 = (sysLifeTime >> 24) & 0xff;
        I2C_Write8_Wait(10, ADDR_EEPROM, EEP_ADDR_LIFETIME_3, u8);
    }

    sysLifeTime_last = sysLifeTime;
}

void ParseLifeTime(unsigned char *hourString, unsigned char *minuteString) {
    uint32_t temp;
    uint32_t hour = sysLifeTime / 360;
    uint32_t minute = (sysLifeTime % 360) / 6;

    SET_ASCII_NON_ZERO(hourString[0], (hour % 10000) / 1000);
    SET_ASCII_NON_ZERO(hourString[1], (hour % 1000) / 100);
    SET_ASCII(hourString[2], (hour % 100) / 10);
    SET_ASCII(hourString[3], (hour % 10));

    SET_ASCII(minuteString[0], (minute % 60) / 10);
    SET_ASCII(minuteString[1], (minute % 10));
}