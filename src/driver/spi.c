#include "spi.h"

#include "config.h"
#include "mcu.h"

#define SET_CS(n) SPI_CS = (n)
#define SET_CK(n) SPI_CK = (n)
#define SET_DO(n) SPI_DO = (n)
#define SET_DI(n) SPI_DI = (n)

#define SPI_DLY \
    {           \
        NOP();  \
        NOP();  \
        NOP();  \
    }

void spi_init() {
    SET_CS(1);
    SET_CK(0);
    SET_DO(0);
    SET_DI(1);
}

uint8_t spi_tranfer_byte(uint8_t data) {
    uint8_t ret = 0;

    for (int8_t i = 7; i >= 0; i--) {
        SPI_DLY;

        SET_DO((data >> i) & 0x01);
        SPI_DLY;

        SET_CK(1);
        SPI_DLY;

        ret = (ret << 1) | SPI_DI;
        SET_CK(0);
    }

    return ret;
}

void spi_write(uint8_t trans, uint16_t addr, uint32_t data) {
    const uint8_t len = trans == 6 ? 2 : trans + 1;

    SET_CS(0);
    SPI_DLY;
    SPI_DLY;

    uint8_t byte = 0x80 | (trans << 4) | (addr >> 8);
    spi_tranfer_byte(byte);

    byte = addr & 0xFF;
    spi_tranfer_byte(byte);

    for (int8_t i = len - 1; i >= 0; i--) {
        byte = (data >> (i << 3)) & 0xFF;
        spi_tranfer_byte(byte);
    }

    SPI_DLY;
    SPI_DLY;
    SET_CK(0);
    SET_DO(0);
    SET_CS(1);
}

void spi_read(uint8_t trans, uint16_t addr, uint32_t *data) {
    const uint8_t len = trans == 6 ? 2 : trans + 1;

    SET_CS(0);
    SPI_DLY;
    SPI_DLY;

    uint8_t byte = 0x00 | (trans << 4) | (addr >> 8);
    spi_tranfer_byte(byte);

    byte = addr & 0xFF;
    spi_tranfer_byte(byte);

    for (int8_t i = len - 1; i >= 0; i--) {
        *data = (*data) << 8;
        *data |= spi_tranfer_byte(0xff);
    }

    SPI_DLY;
    SPI_DLY;
    SET_CK(0);
    SET_DO(0);
    SET_CS(1);
}

void spi_write_reg_map(const spi_reg_value_t *reg_map, const uint8_t size) {
    for (uint8_t i = 0; i < size; i++) {
        spi_write(reg_map[i].trans, reg_map[i].addr, reg_map[i].dat);
    }
}