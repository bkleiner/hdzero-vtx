#include "common.h"
#include "debug.h"
#include "hardware.h"
#include "lifetime.h"
#include "mcu.h"
#include "msp_displayport.h"
#include "osd_display.h"
#include "smartaudio_protocol.h"

#include "driver/uart.h"

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
    mcu_init();

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
        while (sa_task())
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
        sa_timer_task();
#endif
    }
}