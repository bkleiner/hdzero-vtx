#include "eeprom.h"

#include <stddef.h>

#include "driver/i2c_device.h"
#include "driver/time.h"

#include "debug.h"

uint8_t eeprom_content[EEPROM_SIZE];
eeprom_storage_t *eeprom = (eeprom_storage_t *)eeprom_content;

void eeprom_init() {
    const uint8_t ret = i2c_write8(ADDR_EEPROM, EEPROM_ADDR_MAGIC, 0xDE);
    if (ret) {
        debugf("eeprom init success\r\n");
    } else {
        debugf("eeprom init fail\r\n");
    }
}

void eeprom_load() {
    time_delay_ms(10);
    for (uint8_t i = 0; i < EEPROM_SIZE; i++) {
        eeprom_content[i] = i2c_read8(ADDR_EEPROM, i);
        time_delay_ms(1);
    }

#ifdef DEBUG_EEPROM
    debugf("eeprom content: \r\n");
    debug_hex(eeprom_content, EEPROM_SIZE);
    debugf("eeprom magic: %x \r\n", eeprom->magic);
    debugf("\r\n");
#endif
}

void eeprom_save() {
    for (uint8_t i = 0; i < EEPROM_SIZE; i++) {
        i2c_write8(ADDR_EEPROM, i, eeprom_content[i]);
        time_delay_ms(10);
    }
}