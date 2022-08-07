#ifndef __SPI_H_
#define __SPI_H_

#include "stdint.h"

void spi_init();

void spi_write(uint8_t trans, uint16_t addr, uint32_t data);
void spi_read(uint8_t trans, uint16_t addr, uint32_t *data);

#endif /* __SPI_H_ */
