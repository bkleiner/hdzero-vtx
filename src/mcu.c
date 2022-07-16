#include "common.h"
#include "isr.h"
#include "global.h"
#include "uart.h"
#include "print.h"
#include "monitor.h"
#include "sfr_ext.h"
#include "hardware.h"
#include "i2c_device.h"
#include "i2c.h"
#include "rom.h"
#include "msp_displayport.h"
#include "smartaudio_protocol.h"
#include "dm6300.h"
#include "camera.h"
#include "lifetime.h"

void timer_task();
#ifdef USE_SMARTAUDIO
void SA_Delay_init();
#endif


void flash_number(uint16_t number)
{
    uint8_t number_1s = (number / 1) % 10;
    uint8_t number_10s = (number / 10) % 10;
    uint8_t number_100s = (number / 100) % 10;
    uint8_t number_1000s = (number / 1000) % 10;
    while(1)
    {
        LED_BLUE_ON;
        WAIT(1000);
        LED_BLUE_OFF;
        WAIT(1000);
        for(int i = 0; i <= number_1000s; i++)
        {
            LED_BLUE_ON;
            WAIT(200);
            LED_BLUE_OFF;
            WAIT(200);
        }
        LED_BLUE_OFF;
        WAIT(500);
        for(int i = 0; i <= number_100s; i++)
        {
            LED_BLUE_ON;
            WAIT(200);
            LED_BLUE_OFF;
            WAIT(200);
        }
        LED_BLUE_OFF;
        WAIT(500);
        for(int i = 0; i <= number_10s; i++)
        {
            LED_BLUE_ON;
            WAIT(200);
            LED_BLUE_OFF;
            WAIT(200);
        }
        LED_BLUE_OFF;
        WAIT(500);
        for(int i = 0; i <= number_1s; i++)
        {
            LED_BLUE_ON;
            WAIT(200);
            LED_BLUE_OFF;
            WAIT(200);
        }
        LED_BLUE_OFF;
        WAIT(500);
    }
}


void Timer0_isr(void) __interrupt 1
{
    TH0 = 138;

    #ifdef USE_SMARTAUDIO
    if(SA_config) {
        if(suart_tx_en) {
            //TH0 = 139;
            suart_txint();
        }
        else
            suart_rxint();
    }
    #endif

    timer_ms10x++;
}

void Timer1_isr(void) __interrupt 3
{
}


void Ext0_isr(void) __interrupt 0
{
}

void Ext1_isr(void) __interrupt 2
{
}

void UART0_isr() __interrupt 4
{
	if( RI ) {			//RX int
		RI = 0;
		RS_buf[RS_in++] = SBUF0;
		if( RS_in>=BUF_MAX ) RS_in = 0;
        if(RS_in == RS_out) RS0_ERR = 1;
	}

	if( TI ) {			//TX int
		TI = 0;
		RS_Xbusy=0;
	}
}

#define RI1 (SCON1 & 0x01)
#define TI1 (SCON1 & 0x02)

void UART1_isr() __interrupt 6
{
    
	if( RI1 ) {			//RX int
		SCON1 &= 0xfe;
		RS_buf1[RS_in1++] = SBUF1;
		if( RS_in1>=BUF_MAX ) RS_in1 = 0;
	}

	if( TI1 ) {			//TX int
		SCON1 &= 0xfd;
		RS_Xbusy1=0;
	}
}
/*
void UART1_isr() __interrupt 6
{

	if( RI1 ) {			//RX int
		RI1 = 0;
		RS_buf1[RS_in1++] = SBUF1;
		if( RS_in1>=BUF_MAX ) RS_in1 = 0;
	}

	if( TI1 ) {			//TX int
		TI1 = 0;
		RS_Xbusy1=0;
	}
}*/
#define DELAY_Q     {int i=(I2C_BIT_DLY_SDCC>>2); while(i--);  }

void main(void)
{
    RS_in = 0;
    RS_out = 0;
    RS_Xbusy = 0;
    RS_in1 = 0;
    RS_out1 = 0;
    RS_Xbusy1 = 0;

    SA_lock = 0;
    // init
    CPU_init();
    WriteReg(0, 0xB0, 0x3E);
    WriteReg(0, 0xB2, 0x03);
    WriteReg(0, 0x80, 0xC8);
    
    DM6300_variable_init();
    //WAIT(100);
    init_EEP_LifeTime();
    #ifdef _DEBUG_MODE
    Printf("\r\n========================================================");
    Printf("\r\n     >>>             Divimath DM568X            <<<     ");
    Printf("\r\n========================================================");
    Printf("\r\n");
    #endif

    uint16_t cur_timer = timer_ms10x;
    for(int delaycounter = 0; delaycounter < 100; delaycounter++)
    {
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
        DELAY_Q;
    }
    uint16_t new_timer = timer_ms10x;
    uint16_t delaydiff = new_timer - cur_timer;
    Printf("Delay caused this difference: %x", delaydiff);

    
    Init_HW(); // init
    fc_init(); // init displayport
    #ifdef USE_SMARTAUDIO
    SA_Init();
    #endif
    
    #ifdef _DEBUG_MODE
    Prompt();
    #endif

    heat_protect = 0;
    // main loop
    while(1) {

        timer_task();
        #ifdef USE_SMARTAUDIO
        while(SA_task());
        #endif

        #ifdef _RF_CALIB
        CalibProc();
        #elif defined _DEBUG_MODE
        Monitor();
        #endif
        if(heat_protect) {
            flash_number(2);
        }
        Video_Detect();
        if(!SA_lock) OnButton1();
        if((last_SA_lock && (seconds > WAIT_SA_CONFIG)) || (last_SA_lock == 0)){
            TempDetect(); // temperature dectect
            PwrLMT(); // RF power ctrl
            msp_task(); // msp displayport process
            Update_EEP_LifeTime();
        }

        #ifdef USE_SMARTAUDIO
        SA_Delay_init();
        #endif

    }
}

void timer_task()
{
    static uint16_t cur_ms10x_1sd16 = 0;
    static uint16_t last_ms10x_1sd16 = 0;
    static uint8_t  timer_cnt = 0;
    cur_ms10x_1sd16 = timer_ms10x;
    if(((cur_ms10x_1sd16 - last_ms10x_1sd16) >= TIMER0_1SD16) || (cur_ms10x_1sd16 < last_ms10x_1sd16)) {
        last_ms10x_1sd16 = cur_ms10x_1sd16;
        timer_cnt++;
        timer_cnt &= 15;
        if(timer_cnt == 15) {   //every second, 1Hz
            btn1_tflg = 1;
            pwr_tflg = 1;
            cfg_tflg = 1;
            seconds++;
            pwr_sflg = 1;
        }

        if((timer_cnt&7) == 7)  //every half second, 2Hz
            temp_tflg = 1;
        
        if((timer_cnt&3)== 3)   //every quater second, 4Hz
            timer_4hz = 1;
        
        if((timer_cnt&1)== 1)   //every octual second, 8Hz
            timer_8hz = 1;
        timer_16hz = 1;
    }
}

#ifdef USE_SMARTAUDIO
void SA_Delay_init()
{
    static uint8_t SA_saved = 0;
    static uint8_t SA_init_done = 0;
    
    if(SA_saved == 0){
        if(seconds >= WAIT_SA_CONFIG){
            I2C_Write(ADDR_EEPROM, EEP_ADDR_SA_LOCK, SA_lock, 0, 0);
            #ifdef _DEBUG_MODE
            Printf("\r\nSave SA_lock(%bx) to EEPROM",SA_lock);
            #endif
            SA_saved = 1;
        }
    }
    
    //init_rf
    if(SA_init_done==0){
        if(last_SA_lock && seconds >= WAIT_SA_CONFIG){
            #ifdef _DEBUG_MODE
            Printf("\r\nInit RF");
            #endif
            pwr_lmt_sec = PWR_LMT_SEC;
            SA_init_done = 1;
            if(SA_lock){
                if(pwr_init == POWER_MAX+2){ //0mW
                    #if(0)
                    SA_dbm = pwr_to_dbm(pwr_init);
                    RF_POWER = POWER_MAX + 1;
                    Init_6300RF(ch_init, RF_POWER);
                    PIT_MODE = PIT_0MW;
                    #ifdef _DEBUG_MODE
                    Printf("\r\n SA_dbm:%bx",SA_dbm);
                    Printf("\r\n ch%bx, pwr%bx", ch_init, pwr_init);
                    #endif
                    #else
                    RF_POWER = POWER_MAX + 2;
                    cur_pwr = POWER_MAX + 2;
                    #endif
                }else if(PIT_MODE){
                    Init_6300RF(ch_init, POWER_MAX + 1);
                    #ifdef _DEBUG_MODE
                    Printf("\r\n ch%bx, pwr%bx", ch_init, cur_pwr);
                    #endif
                }else{
                    Init_6300RF(ch_init, pwr_init);
                    #ifdef _DEBUG_MODE
                    Printf("\r\n ch%bx, pwr%bx", ch_init, cur_pwr);
                    #endif
                }
            }else if(PIT_MODE){
                Init_6300RF(RF_FREQ, POWER_MAX+1);
                #ifdef _DEBUG_MODE
                Printf("\r\n ch%bx, pwr%bx", RF_FREQ, cur_pwr);
                #endif
            }else{
                Init_6300RF(RF_FREQ, 0);
                #ifdef _DEBUG_MODE
                Printf("\r\n ch%bx, pwr%bx", RF_FREQ, 0);
                #endif
            }
            
            DM6300_AUXADC_Calib();
        }
    }
}
#endif
