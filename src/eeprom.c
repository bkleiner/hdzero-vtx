#include "eeprom.h"

#include <stddef.h>

#include "driver/i2c_device.h"

#include "debug.h"

uint8_t eeprom_content[EEPROM_SIZE];
eeprom_storage_t *storage = (eeprom_storage_t *)eeprom_content;

void eeprom_init() {
    const uint8_t ret = i2c_write8(ADDR_EEPROM, EEPROM_ADDR_MAGIC, 0xDE);
    if (ret) {
        debugf("eeprom init success\r\n");
    } else {
        debugf("eeprom init fail\r\n");
    }
}

void eeprom_load() {
    for (uint8_t i = 0; i < EEPROM_SIZE; i++) {
        eeprom_content[i] = i2c_read8(ADDR_EEPROM, i);
    }

#ifdef DEBUG_EEPROM
    debugf("eeprom content: \r\n");
    for (uint16_t i = 0; i < EEPROM_SIZE;) {
        debugf("%02X: ", i);
        debugf("%02X%02X%02X%02X", eeprom_content[i++], eeprom_content[i++], eeprom_content[i++], eeprom_content[i++]);
        debugf(" ");
        debugf("%02X%02X%02X%02X", eeprom_content[i++], eeprom_content[i++], eeprom_content[i++], eeprom_content[i++]);
        debugf(" ");
        debugf("%02X%02X%02X%02X", eeprom_content[i++], eeprom_content[i++], eeprom_content[i++], eeprom_content[i++]);
        debugf(" ");
        debugf("%02X%02X%02X%02X", eeprom_content[i++], eeprom_content[i++], eeprom_content[i++], eeprom_content[i++]);
        debugf("\r\n");
    }
    debugf("eeprom magic: %x \r\n", storage->magic);
    debugf("\r\n");
#endif
}

void eeprom_save() {
    for (uint8_t i = 0; i < EEPROM_SIZE; i++) {
        i2c_write8(ADDR_EEPROM, i, eeprom_content[i]);
    }
}