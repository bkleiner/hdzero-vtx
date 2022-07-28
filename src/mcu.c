#include "mcu.h"

#include "camera.h"
#include "common.h"
#include "debug.h"
#include "dm6300.h"
#include "driver/i2c.h"
#include "driver/i2c_device.h"
#include "driver/register.h"
#include "driver/uart.h"
#include "global.h"
#include "hardware.h"
#include "lifetime.h"
#include "msp_displayport.h"
#include "osd_display.h"
#include "rom.h"
#include "smartaudio_protocol.h"

BIT_TYPE btn1_tflg = 0;
BIT_TYPE pwr_sflg = 0; // power autoswitch flag
BIT_TYPE pwr_tflg = 0;
BIT_TYPE cfg_tflg = 0;
BIT_TYPE temp_tflg = 0;
BIT_TYPE timer_4hz = 0;
BIT_TYPE timer_8hz = 0;
BIT_TYPE timer_16hz = 0;
BIT_TYPE RS0_ERR = 0;
IDATA_SEG volatile uint16_t timer_ms10x = 0;
uint16_t seconds = 0;

void mcu_init() {
    SCON0 = 0x50; // [7:6] uart0 mode: 0x01 = 8bit, variable baudrate(related to timer1)
                  // [4]   uart0 rx enable

    SCON1 = 0x50; // [7:6] uart1 mode: 0x01 = 8bit, variable baudrate(related to timer1)
                  // [4]   uart1 rx enable

    PCON = 0xC0; // [7]   double rate for uart0
                 // [6]   double rate for uart1

    TMOD = 0x20; // [7]   timer1 enabled by pin gate1
                 // [6]   timer1: 0=as timer, 1=as counter
                 // [5:4] tmier1 mode: 0x10 = 8bit timer with 8bit auto-reload
                 // [3]   timer0 enabled by pin gate0
                 // [2]   timer0: 0=as timer, 1=as counter
                 // [1:0] tmier0 mode: 0x01 = 16bit timer, {TH0,TL0} cascaded

    CKCON = 0x1F; // [4]   timer1 uses a divide-by-N of the system clock frequency. 0:N=12; 1:N=4
                  // [3]   timer0 uses a divide-by-N of the system clock frequency. 0:N=12; 1:N=4

    TH0 = 139;
    TL0 = 0;

    TH1 = 0xEC; // [7:0] in timer mode 0x10:   ----------------->> 148.5MHz: 0x87; 100MHz: 0xAF; 54MHz: 0xD4; 27MHz: 0xEA
                //	               f(clk)
                //  BaudRate = --------------  (M=16 or 32, decided by PCON double rate flag)
                //             N*(256-TH1)*M   (N=4 or 12, decided by CKCON [4])
                // 19200  - 0x87
                // 115200 - 0xEC    256 - 148.5M/64/115200 = 0xEC  0.7%
                // 230400 - 0xF6    256 - 148.5M/64/230400 = 0xF6  0.7%
                // 250000 - 0xF7    256 - 148.5M/64/250000 = 0xF7  3.125%

    TCON = 0x50; // [6]   enable timer1
                 // [4]   enable timer0

    IE = 0xD2; // [7]   enable global interupts  1
               // [6]   enable uart1  interupt   1
               // [5]   enable timer2 interupt   0
               // [4]   enable uart0  interupt   1
               // [3]   enable timer1 interupt   0
               // [2]   enable INT1   interupt   0
               // [1]   enable timer0 interupt   0
               // [0]   enable INT0   interupt   0
    IP = 0x10; // UART0=higher priority, Timer 0 = low

    WriteReg(0, 0xB0, 0x3E);
    WriteReg(0, 0xB2, 0x03);
    WriteReg(0, 0x80, 0xC8);
}

void timer_task() {
    static uint16_t cur_ms10x_1sd16 = 0, last_ms10x_1sd16 = 0;
    static uint8_t timer_cnt = 0;

    // reset timers from last loop
    if (timer_4hz) {
        timer_4hz = 0;
    }

    if (timer_8hz) {
        timer_8hz = 0;
    }

    cur_ms10x_1sd16 = timer_ms10x;

    if (((cur_ms10x_1sd16 - last_ms10x_1sd16) >= TIMER0_1SD16) || (cur_ms10x_1sd16 < last_ms10x_1sd16)) {
        last_ms10x_1sd16 = cur_ms10x_1sd16;
        timer_cnt++;
        timer_cnt &= 15;

        if (timer_cnt == 15) { // every second, 1Hz
            btn1_tflg = 1;
            pwr_tflg = 1;
            cfg_tflg = 1;
            seconds++;
            pwr_sflg = 1;
        }

        if ((timer_cnt & 7) == 7) // every half second, 2Hz
            temp_tflg = 1;

        if ((timer_cnt & 3) == 3) // every quater second, 4Hz
            timer_4hz = 1;

        if ((timer_cnt & 1) == 1) // every octual second, 8Hz
            timer_8hz = 1;

        timer_16hz = 1;
    }
}

#ifdef USE_SMARTAUDIO
void sa_timer_task() {
    static uint8_t SA_saved = 0;
    static uint8_t SA_init_done = 0;

    if (SA_saved == 0) {
        if (seconds >= WAIT_SA_CONFIG) {
            I2C_Write8(ADDR_EEPROM, EEP_ADDR_SA_LOCK, SA_lock);
            debugf("\r\nSave SA_lock(%x) to EEPROM", (uint16_t)SA_lock);
            SA_saved = 1;
        }
    }

    // init_rf
    if (SA_init_done == 0) {
        if (last_SA_lock && seconds >= WAIT_SA_CONFIG) {
            debugf("\r\nInit RF");
            pwr_lmt_sec = PWR_LMT_SEC;
            SA_init_done = 1;
            if (SA_lock) {
                if (pwr_init == POWER_MAX + 2) { // 0mW
#if (0)
                    SA_dbm = pwr_to_dbm(pwr_init);
                    RF_POWER = POWER_MAX + 1;
                    Init_6300RF(ch_init, RF_POWER);
                    PIT_MODE = PIT_0MW;
                    debugf("\r\n SA_dbm:%x", (uint16_t)SA_dbm);
                    debugf("\r\n ch%x, pwr%x", (uint16_t)ch_init, (uint16_t)pwr_init);
#else
                    RF_POWER = POWER_MAX + 2;
                    cur_pwr = POWER_MAX + 2;
#endif
                } else if (PIT_MODE) {
                    Init_6300RF(ch_init, POWER_MAX + 1);
                    debugf("\r\n ch%x, pwr%x", (uint16_t)ch_init, (uint16_t)cur_pwr);
                } else {
                    Init_6300RF(ch_init, pwr_init);
                    debugf("\r\n ch%x, pwr%x", (uint16_t)ch_init, (uint16_t)cur_pwr);
                }
            } else if (PIT_MODE) {
                Init_6300RF(RF_FREQ, POWER_MAX + 1);
                debugf("\r\n ch%x, pwr%x", (uint16_t)RF_FREQ, (uint16_t)cur_pwr);
            } else {
                Init_6300RF(RF_FREQ, 0);
                debugf("\r\n ch%x, pwr%x", (uint16_t)RF_FREQ, 0);
            }

            DM6300_AUXADC_Calib();
        }
    }
}
#endif
