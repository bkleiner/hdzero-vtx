#ifndef __SFR_DEF_H_
#define __SFR_DEF_H_

#include "toolchain.h"

////////////////////////////////////////////////////////////////////////////////
// SFR

SFR_DEF(P0,        0x80);   /* Port 0                    */
SFR_DEF(SP,        0x81);   /* Stack Pointer             */
SFR_DEF(DPL0,      0x82);   /* Data Pointer 0 Low byte   */
SFR_DEF(DPH0,      0x83);   /* Data Pointer 0 High byte  */
SFR_DEF(DPL1,      0x84);   /* Data Pointer 1 Low byte   */
SFR_DEF(DPH1,      0x85);   /* Data Pointer 1 High byte  */
SFR_DEF(DPS,       0x86);
SFR_DEF(PCON,      0x87);   /* Power Configuration       */

SFR_DEF(TCON,      0x88);   /* Timer 0,1 Configuration   */
SFR_DEF(TMOD,      0x89);   /* Timer 0,1 Mode            */
SFR_DEF(TL0,       0x8A);   /* Timer 0 Low byte counter  */
SFR_DEF(TL1,       0x8B);   /* Timer 1 Low byte counter  */
SFR_DEF(TH0,       0x8C);   /* Timer 0 High byte counter */
SFR_DEF(TH1,       0x8D);   /* Timer 1 High byte counter */
SFR_DEF(CKCON,     0x8E);   /* XDATA Wait States         */

SFR_DEF(P1,        0x90);   /* Port 1                    */
SFR_DEF(EIF,       0x91);
SFR_DEF(WTST,      0x92);   /* Program Wait States       */
SFR_DEF(DPX0,      0x93);   /* Data Page Pointer 0       */
SFR_DEF(DPX1,      0x95);   /* Data Page Pointer 1       */

SFR_DEF(SCON0,     0x98);   /* Serial 0 Configuration    */
SFR_DEF(SBUF0,     0x99);   /* Serial 0 I/O Buffer       */

SFR_DEF(P2,        0xA0);   /* Port 2                    */

SFR_DEF(IE,        0xA8);   /* Interrupt Enable          */

SFR_DEF(P3,        0xB0);   /* Port 3                    */

SFR_DEF(IP,        0xB8);

SFR_DEF(SCON1,     0xC0);   /* Serial 1 Configuration    */
SFR_DEF(SBUF1,     0xC1);   /* Serial 1 I/O Buffer       */

SFR_DEF(T2CON,     0xC8);
SFR_DEF(T2IF,      0xC9);
SFR_DEF(RLDL,      0xCA);
SFR_DEF(RLDH,      0xCB);
SFR_DEF(TL2,       0xCC);
SFR_DEF(TH2,       0xCD);
SFR_DEF(CCEN,      0xCE);

SFR_DEF(PSW,       0xD0);   /* Program Status Word       */

SFR_DEF(WDCON,     0xD8);

SFR_DEF(ACC,       0xE0);   /* Accumulator               */

SFR_DEF(EIE,       0xE8);   /* External Interrupt Enable */
SFR_DEF(STATUS,    0xE9);   /* Status register           */
SFR_DEF(MXAX,      0xEA);   /* MOVX @Ri High address     */
SFR_DEF(TA,        0xEB);

SFR_DEF(B,         0xF0);   /* B Working register        */

SFR_DEF(EIP,       0xF8);

////////////////////////////////////////////////////////////////////////////////
// Bit define

/*  P0  */
SBIT_DEF(P0_7,     (0x80 + 7));
SBIT_DEF(P0_6,     (0x80 + 6));
SBIT_DEF(P0_5,     (0x80 + 5));
SBIT_DEF(P0_4,     (0x80 + 4));
SBIT_DEF(P0_3,     (0x80 + 3));
SBIT_DEF(P0_2,     (0x80 + 2));
SBIT_DEF(P0_1,     (0x80 + 1));
SBIT_DEF(P0_0,     (0x80 + 0));

/*  P1  */
SBIT_DEF(P1_7,     (0x90 + 7));
SBIT_DEF(P1_6,     (0x90 + 6));
SBIT_DEF(P1_5,     (0x90 + 5));
SBIT_DEF(P1_4,     (0x90 + 4));
SBIT_DEF(P1_3,     (0x90 + 3));
SBIT_DEF(P1_2,     (0x90 + 2));
SBIT_DEF(P1_1,     (0x90 + 1));
SBIT_DEF(P1_0,     (0x90 + 0));

/*  P2  */
SBIT_DEF(P2_7,     (0xA0 + 7));
SBIT_DEF(P2_6,     (0xA0 + 6));
SBIT_DEF(P2_5,     (0xA0 + 5));
SBIT_DEF(P2_4,     (0xA0 + 4));
SBIT_DEF(P2_3,     (0xA0 + 3));
SBIT_DEF(P2_2,     (0xA0 + 2));
SBIT_DEF(P2_1,     (0xA0 + 1));
SBIT_DEF(P2_0,     (0xA0 + 0));

/*  P3  */
SBIT_DEF(P3_7,     (0xB0 + 7));
SBIT_DEF(P3_6,     (0xB0 + 6));
SBIT_DEF(P3_5,     (0xB0 + 5));
SBIT_DEF(P3_4,     (0xB0 + 4));
SBIT_DEF(P3_3,     (0xB0 + 3));
SBIT_DEF(P3_2,     (0xB0 + 2));
SBIT_DEF(P3_1,     (0xB0 + 1));
SBIT_DEF(P3_0,     (0xB0 + 0));

/*  TCON  */
SBIT_DEF(TF1,      (0x88 + 7));
SBIT_DEF(TR1,      (0x88 + 6));
SBIT_DEF(TF0,      (0x88 + 5));
SBIT_DEF(TR0,      (0x88 + 4));
SBIT_DEF(IE1,      (0x88 + 3));
SBIT_DEF(IT1,      (0x88 + 2));
SBIT_DEF(IE0,      (0x88 + 1));
SBIT_DEF(IT0,      (0x88 + 0));

/*  IE  */
SBIT_DEF(EA,       (0xA8 + 7));
SBIT_DEF(ES1,      (0xA8 + 6));
SBIT_DEF(ET2,      (0xA8 + 5));
SBIT_DEF(ES0,      (0xA8 + 4));
SBIT_DEF(ET1,      (0xA8 + 3));
SBIT_DEF(EX1,      (0xA8 + 2));
SBIT_DEF(ET0,      (0xA8 + 1));
SBIT_DEF(EX0,      (0xA8 + 0));

/*  IP  */
SBIT_DEF(PS1,      (0xB8 + 6));
SBIT_DEF(PT2,      (0xB8 + 5));
SBIT_DEF(PS0,      (0xB8 + 4));
SBIT_DEF(PT1,      (0xB8 + 3));
SBIT_DEF(PX1,      (0xB8 + 2));
SBIT_DEF(PT0,      (0xB8 + 1));
SBIT_DEF(PX0,      (0xB8 + 0));

/*  SCON0  */
SBIT_DEF(SM0,      (0x98 + 7));
SBIT_DEF(SM1,      (0x98 + 6));
SBIT_DEF(SM2,      (0x98 + 5));
SBIT_DEF(REN,      (0x98 + 4));
SBIT_DEF(TB8,      (0x98 + 3));
SBIT_DEF(RB8,      (0x98 + 2));
SBIT_DEF(TI,       (0x98 + 1));
SBIT_DEF(RI,       (0x98 + 0));

/*  SCON1  */
SBIT_DEF(SM10,     (0xC0 + 7));
SBIT_DEF(SM11,     (0xC0 + 6));
SBIT_DEF(SM12,     (0xC0 + 5));
SBIT_DEF(REN1,     (0xC0 + 4));
SBIT_DEF(TB18,     (0xC0 + 3));
SBIT_DEF(RB18,     (0xC0 + 2));
SBIT_DEF(TI1,      (0xC0 + 1));
SBIT_DEF(RI1,      (0xC0 + 0));

/*  EIF  */
//SBIT_DEF(INT6F,    EIF^4);
//SBIT_DEF(INT5F,    EIF^3);
//SBIT_DEF(INT4F,    EIF^2);
//SBIT_DEF(INT3F,    EIF^1);
//SBIT_DEF(INT2F,    EIF^0);

#endif
