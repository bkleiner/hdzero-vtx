#ifndef __I2C_H_
#define __I2C_H_

#include "stdint.h"

void i2c_init();

uint8_t i2c_write8(uint8_t slave_addr, uint8_t reg_addr, uint8_t val);
uint8_t i2c_write16(uint8_t slave_addr, uint16_t reg_addr, uint16_t val);

uint8_t i2c_read8(uint8_t slave_addr, uint8_t reg_addr);
uint16_t i2c_read16(uint8_t slave_addr, uint16_t reg_addr);

#endif /* __I2C_H_ */
