#include "i2c_device.h"

#include "config.h"
#include "mcu.h"
#include "time.h"

void tc3587_init() {
#ifdef TC3587_RST_PIN
    TC3587_RST_PIN = 0;
    time_delay_ms(80);
    TC3587_RST_PIN = 1;
    time_delay_ms(20);
#endif

    i2c_write16(ADDR_TC3587, 0x0002, 0x0001);
    i2c_write16(ADDR_TC3587, 0x0002, 0x0000); // srst

    i2c_write16(ADDR_TC3587, 0x0004, 0x026f); // config

    i2c_write16(ADDR_TC3587, 0x0006, 0x0062); // fifo
    i2c_write16(ADDR_TC3587, 0x0008, 0x0061); // data format

    i2c_write16(ADDR_TC3587, 0x0060, 0x800a); // mipi phy timing delay ; 0x800a

    i2c_write16(ADDR_TC3587, 0x0018, 0x0111); // pll
    i2c_write16(ADDR_TC3587, 0x0018, 0x0113); // pll
    i2c_write16(ADDR_TC3587, 0x0016, 0x3057); // pll
    i2c_write16(ADDR_TC3587, 0x0020, 0x0000); // clk config
    i2c_write16(ADDR_TC3587, 0x000c, 0x0101); // mclk
    i2c_write16(ADDR_TC3587, 0x0018, 0x0111); // pll //0111
    i2c_write16(ADDR_TC3587, 0x0018, 0x0113); // pll //0113
    i2c_write16(ADDR_TC3587, 0x0002, 0x0001);
    i2c_write16(ADDR_TC3587, 0x0002, 0x0000);
}