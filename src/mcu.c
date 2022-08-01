#include "camera.h"
#include "common.h"
#include "debug.h"
#include "dm6300.h"
#include "global.h"
#include "hardware.h"
#include "i2c.h"
#include "i2c_device.h"
#include "isr.h"
#include "lifetime.h"
#include "msp_displayport.h"
#include "osd_display.h"
#include "register.h"
#include "rom.h"
#include "smartaudio_protocol.h"
#include "uart.h"

uint8_t UNUSED = 0;

void timer_task();
#ifdef USE_SMARTAUDIO
void SA_Delay_init();
#endif

BIT_TYPE int0_req = 0;
BIT_TYPE int1_req = 0;

void Timer0_isr(void) INTERRUPT(1) {
    TH0 = 138;

#ifdef USE_SMARTAUDIO
    if (SA_config) {
        if (suart_tx_en) {
            // TH0 = 139;
            suart_txint();
        } else
            suart_rxint();
    }
#endif

    timer_ms10x++;
}

void Timer1_isr(void) INTERRUPT(3) {
}

void Ext0_isr(void) INTERRUPT(0) {
    int0_req = 1;
}

void Ext1_isr(void) INTERRUPT(2) {
    int1_req = 1;
}

void UART0_isr() INTERRUPT(4) {
    if (RI) { // RX int
        RI = 0;
        RS_buf[RS_in++] = SBUF0;
        if (RS_in >= BUF_MAX)
            RS_in = 0;
        if (RS_in == RS_out)
            RS0_ERR = 1;
    }

    if (TI) { // TX int
        TI = 0;
        RS_Xbusy = 0;
    }
}

void UART1_isr() INTERRUPT(6) {

    if (RI1) { // RX int
        RI1 = 0;
        RS_buf1[RS_in1++] = SBUF1;
        if (RS_in1 >= BUF1_MAX)
            RS_in1 = 0;
    }

    if (TI1) { // TX int
        TI1 = 0;
        RS_Xbusy1 = 0;
    }
}

void main(void) {
    // init
    CPU_init();
    WriteReg(0, 0xB0, 0x3E);
    WriteReg(0, 0xB2, 0x03);
    WriteReg(0, 0x80, 0xC8);
    // WAIT(100);

    debugf("\r\n========================================================");
    debugf("\r\n     >>>             Divimath DM568X            <<<     ");
    debugf("\r\n========================================================");
    debugf("\r\n");

    Init_HW(); // init

    osd_init();
    msp_init();

#ifdef USE_SMARTAUDIO
    SA_Init();
#endif

#ifdef _DEBUG_MODE
    debug_prompt();
#endif

    // main loop
    while (1) {
        timer_task();

#ifdef USE_SMARTAUDIO
        while (SA_task())
            ;
#endif

#ifdef _RF_CALIB
        CalibProc();
#elif defined _DEBUG_MODE
        debug_monitor();
#endif

        Video_Detect();
        if (!SA_lock)
            OnButton1();

        if ((last_SA_lock && (seconds > WAIT_SA_CONFIG)) || (last_SA_lock == 0)) {
            TempDetect(); // temperature dectect
            PwrLMT();     // RF power ctrl

            msp_task(); // msp displayport process
            osd_task();

            Update_EEP_LifeTime();
        }
#ifdef USE_SMARTAUDIO
        SA_Delay_init();
#endif
    }
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
void SA_Delay_init() {
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
