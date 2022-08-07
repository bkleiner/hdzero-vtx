#include "i2c.h"

#include "config.h"
#include "mcu.h"

#define SCL_SET(n) SCL = n
#define SDA_SET(n) SDA = n

#define SCL_GET() SCL
#define SDA_GET() SDA

#define DELAY_Q delay_10us()

void delay_10us() {
    __asm__(
        "mov r7,#91\n"
        "00000$:\n"
        "djnz r7,00000$\n");
}

void i2c_init() {
    SDA_SET(0);
    SCL_SET(0);
}

void i2c_start() {
    SDA_SET(1);
    DELAY_Q;

    SCL_SET(1);
    DELAY_Q;

    SDA_SET(0);
    DELAY_Q;
    DELAY_Q;

    SCL_SET(0);
    DELAY_Q;
}

void i2c_stop() {
    SDA_SET(0);
    DELAY_Q;

    SCL_SET(1);
    DELAY_Q;

    SDA_SET(1);
    DELAY_Q;
    DELAY_Q;
}

uint8_t i2c_write_byte(uint8_t val) {
    for (uint8_t i = 0; i < 8; i++) {
        if (val >> 7)
            SDA_SET(1);
        else
            SDA_SET(0);
        DELAY_Q;

        SCL_SET(1);
        DELAY_Q;
        DELAY_Q;

        SCL_SET(0);
        DELAY_Q;

        val <<= 1;
    }

    SDA_SET(1);
    DELAY_Q;

    SCL_SET(1);
    uint8_t ret = SDA_GET();
    DELAY_Q;
    DELAY_Q;

    SCL_SET(0);
    DELAY_Q;

    return ret;
}

uint8_t i2c_read_byte(uint8_t no_ack) {
    uint8_t val = 0;
    for (uint8_t i = 0; i < 8; i++) {
        DELAY_Q;
        SCL_SET(1);

        val <<= 1;
        val |= SDA_GET();

        DELAY_Q;
        DELAY_Q;

        SCL_SET(0);
        DELAY_Q;
    }

    // master ack
    SDA_SET(no_ack);
    DELAY_Q;

    SCL_SET(1);
    DELAY_Q;
    DELAY_Q;

    SCL_SET(0);
    DELAY_Q;

    SDA_SET(1);

    return val;
}

uint8_t i2c_write8(uint8_t slave_addr, uint8_t reg_addr, uint8_t val) {
    slave_addr = slave_addr << 1;

    i2c_start();

    const uint8_t ret = i2c_write_byte(slave_addr);
    if (ret) {
        i2c_stop();
        return 0;
    }

    i2c_write_byte(reg_addr);
    i2c_write_byte(val);

    i2c_stop();

    return 1;
}

uint8_t i2c_write16(uint8_t slave_addr, uint16_t reg_addr, uint16_t val) {
    slave_addr = slave_addr << 1;

    i2c_start();

    const uint8_t ret = i2c_write_byte(slave_addr);
    if (ret) {
        i2c_stop();
        return 0;
    }

    uint8_t tmp = reg_addr >> 8;
    i2c_write_byte(tmp);
    tmp = reg_addr & 0xFF;
    i2c_write_byte(tmp);

    // data
    tmp = val >> 8;
    i2c_write_byte(tmp);
    tmp = val;
    i2c_write_byte(tmp);

    i2c_stop();

    return 1;
}

uint8_t i2c_read8(uint8_t slave_addr, uint8_t reg_addr) {
    slave_addr = slave_addr << 1;

    i2c_start();
    i2c_write_byte(slave_addr);
    i2c_write_byte(reg_addr);

    i2c_start();
    i2c_write_byte(slave_addr | 0x01);
    uint8_t val = i2c_read_byte(1);
    i2c_stop();

    return val;
}

uint16_t i2c_read16(uint8_t slave_addr, uint16_t reg_addr) {
    slave_addr = slave_addr << 1;

    i2c_start();
    i2c_write_byte(slave_addr);

    uint8_t temp = reg_addr >> 8;
    i2c_write_byte(temp);
    temp = reg_addr & 0xFF;
    i2c_write_byte(temp);

    i2c_start();
    i2c_write_byte(slave_addr | 0x01);

    // data
    temp = i2c_read_byte(1);
    uint16_t value = temp;
    temp = i2c_read_byte(0);
    value = (value << 8) | temp;
    i2c_stop();

    return value;
}
