#ifndef __SPI_H_
#define __SPI_H_

#include "stdint.h"

typedef struct {
    uint8_t trans;
    uint16_t addr;
    uint32_t dat;
} spi_reg_value_t;

#define SPI_REG_MAP_COUNT(regs) sizeof(regs) / sizeof(spi_reg_value_t)
#define SPI_WRITE_REG_MAP(regs) spi_write_reg_map(regs, SPI_REG_MAP_COUNT(regs))

void spi_init();

void spi_write(uint8_t trans, uint16_t addr, uint32_t data);
void spi_read(uint8_t trans, uint16_t addr, uint32_t *data);

void spi_write_reg_map(const spi_reg_value_t *reg_map, const uint8_t size);

#endif /* __SPI_H_ */
