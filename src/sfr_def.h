#ifndef __SFR_DEF_H_
#define __SFR_DEF_H_

////////////////////////////////////////////////////////////////////////////////
// SFR

__sfr __at (0x80) P0;/* Port 0                    */
__sfr __at (0x81) SP;/* Stack Pointer             */
__sfr __at (0x82) DPL0;/* Data Pointer 0 Low byte   */
__sfr __at (0x83) DPH0;/* Data Pointer 0 High byte  */
__sfr __at (0x84) DPL1;/* Data Pointer 1 Low byte   */
__sfr __at (0x85) DPH1;/* Data Pointer 1 High byte  */
__sfr __at (0x86) DPS;
__sfr __at (0x87) PCON;/* Power Configuration       */

__sfr __at (0x88) TCON;/* Timer 0,1 Configuration   */
__sfr __at (0x89) TMOD;/* Timer 0,1 Mode            */
__sfr __at (0x8A) TL0;/* Timer 0 Low byte counter  */
__sfr __at (0x8B) TL1;/* Timer 1 Low byte counter  */
__sfr __at (0x8C) TH0;/* Timer 0 High byte counter */
__sfr __at (0x8D) TH1;/* Timer 1 High byte counter */
__sfr __at (0x8E) CKCON;/* XDATA Wait States         */

__sfr __at (0x90) P1;/* Port 1                    */
__sfr __at (0x91) EIF;
__sfr __at (0x92) WTST;/* Program Wait States       */
__sfr __at (0x93) DPX0;/* Data Page Pointer 0       */
__sfr __at (0x95) DPX1;/* Data Page Pointer 1       */

__sfr __at (0x98) SCON0;/* Serial 0 Configuration    */
__sfr __at (0x99) SBUF0;/* Serial 0 I/O Buffer       */

__sfr __at (0xA0) P2;/* Port 2                    */

__sfr __at (0xA8) IE;/* Interrupt Enable          */

__sfr __at (0xB0) P3;/* Port 3                    */

__sfr __at (0xB8) IP;

__sfr __at (0xC0) SCON1;/* Serial 1 Configuration    */
__sfr __at (0xC1) SBUF1;/* Serial 1 I/O Buffer       */

__sfr __at (0xC8) T2CON;
__sfr __at (0xC9) T2IF;
__sfr __at (0xCA) RLDL;
__sfr __at (0xCB) RLDH;
__sfr __at (0xCC) TL2;
__sfr __at (0xCD) TH2;
__sfr __at (0xCE) CCEN;

__sfr __at (0xD0) PSW;/* Program Status Word       */

__sfr __at (0xD8) WDCON;

__sfr __at (0xE0) ACC;/* Accumulator               */

__sfr __at (0xE8) EIE;/* External Interrupt Enable */
__sfr __at (0xE9) STATUS;/* Status register           */
__sfr __at (0xEA) MXAX;/* MOVX @Ri High address     */
__sfr __at (0xEB) TA;

__sfr __at (0xF0) B;/* B Working register        */

__sfr __at (0xF8) EIP;

////////////////////////////////////////////////////////////////////////////////
// Bit define

/*  P0  */
__sbit __at (0x80+7) P0_7;
__sbit __at (0x80+6) P0_6;
__sbit __at (0x80+5) P0_5;
__sbit __at (0x80+4) P0_4;
__sbit __at (0x80+3) P0_3;
__sbit __at (0x80+2) P0_2;
__sbit __at (0x80+1) P0_1;
__sbit __at (0x80+0) P0_0;

/*  P1  */
__sbit __at (0x90+7) P1_7;
__sbit __at (0x90+6) P1_6;
__sbit __at (0x90+5) P1_5;
__sbit __at (0x90+4) P1_4;
__sbit __at (0x90+3) P1_3;
__sbit __at (0x90+2) P1_2;
__sbit __at (0x90+1) P1_1;
__sbit __at (0x90+0) P1_0;

/*  P2  */
__sbit __at (0xA0+7) P2_7;
__sbit __at (0xA0+6) P2_6;
__sbit __at (0xA0+5) P2_5;
__sbit __at (0xA0+4) P2_4;
__sbit __at (0xA0+3) P2_3;
__sbit __at (0xA0+2) P2_2;
__sbit __at (0xA0+1) P2_1;
__sbit __at (0xA0+0) P2_0;

/*  P3  */
__sbit __at (0xB0+7) P3_7;
__sbit __at (0xB0+6) P3_6;
__sbit __at (0xB0+5) P3_5;
__sbit __at (0xB0+4) P3_4;
__sbit __at (0xB0+3) P3_3;
__sbit __at (0xB0+2) P3_2;
__sbit __at (0xB0+1) P3_1;
__sbit __at (0xB0+0) P3_0;

/*  TCON  */
__sbit __at (0x88+7) TF1;
__sbit __at (0x88+6) TR1;
__sbit __at (0x88+5) TF0;
__sbit __at (0x88+4) TR0;
__sbit __at (0x88+3) IE1;
__sbit __at (0x88+2) IT1;
__sbit __at (0x88+1) IE0;
__sbit __at (0x88+0) IT0;

/*  IE  */
__sbit __at (0xA8+7) EA;
__sbit __at (0xA8+6) ES1;
__sbit __at (0xA8+5) ET2;
__sbit __at (0xA8+4) ES0;
__sbit __at (0xA8+3) ET1;
__sbit __at (0xA8+2) EX1;
__sbit __at (0xA8+1) ET0;
__sbit __at (0xA8+0) EX0;

/*  IP  */
__sbit __at (0xB8+6) PS1;
__sbit __at (0xB8+5) PT2;
__sbit __at (0xB8+4) PS0;
__sbit __at (0xB8+3) PT1;
__sbit __at (0xB8+2) PX1;
__sbit __at (0xB8+1) PT0;
__sbit __at (0xB8+0) PX0;

/*  SCON0  */
__sbit __at (0x98+7) SM0;
__sbit __at (0x98+6) SM1;
__sbit __at (0x98+5) SM2;
__sbit __at (0x98+4) REN;
__sbit __at (0x98+3) TB8;
__sbit __at (0x98+2) RB8;
__sbit __at (0x98+1) TI;
__sbit __at (0x98+0) RI;

/*  SCON1  */
__sbit __at (0xC0+7) SM10;
__sbit __at (0xC0+6) SM11;
__sbit __at (0xC0+5) SM12;
__sbit __at (0xC0+4) REN1;
__sbit __at (0xC0+3) TB18;
__sbit __at (0xC0+2) RB18;
__sbit __at (0xC0+1) TI1;
__sbit __at (0xC0+0) RI1;

/*  EIF  */
//sbit INT6F    = EIF^4;
//sbit INT5F    = EIF^3;
//sbit INT4F    = EIF^2;
//sbit INT3F    = EIF^1;
//sbit INT2F    = EIF^0;

#endif
