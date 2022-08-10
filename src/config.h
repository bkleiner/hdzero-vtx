#ifndef __CONFIG_H_
#define __CONFIG_H_

#define VERSION 0x41
#define BETA 0x03

#define DEBUG_MODE
//#define REVERSE_UARTS

#ifdef DEBUG_MODE
#define DEBUG_EEPROM
#define DEBUG_EFUSE
#endif

#ifndef VTX_S
#define USE_SMARTAUDIO
#endif

// ------ hardware ------
#define MCU_IS_RX 0
#define DM6300_BW DM6300_BW_27M

#define FREQ_MAX 8
#define FREQ_MAX_EXT 10

#if defined(VTX_L)
#define POWER_MAX 4
#define PIT_POWER 0x18
#elif defined(VTX_M)
#define POWER_MAX 3
#define PIT_POWER 0x26
#else
#define POWER_MAX 2
#define PIT_POWER 0x26
#endif

#ifdef REVERSE_UARTS
#define DEBUG_UART uart0
#define MSP_UART uart1
#else
#define DEBUG_UART uart1
#define MSP_UART uart0
#endif

// ------ pins ------
#define LED_1_PIN P0_2

#ifdef USE_SMARTAUDIO
#define SUART_PORT P0_3
#else
#define TC3587_RST_PIN P0_3
#endif

#define SPI_CS P1_0
#define SPI_CK P1_1
#define SPI_DI P0_6
#define SPI_DO P0_7

#define SCL P0_0
#define SDA P0_1

// ------ ids ------
#define VTX_B_ID 0x4C
#define VTX_M_ID 0x50
#define VTX_S_ID 0x54
#define VTX_R_ID 0x58
#define VTX_WL_ID 0x59
#define VTX_L_ID 0x5C

#if defined(VTX_B)
#define VTX_ID VTX_B_ID
#elif defined(VTX_M)
#define VTX_ID VTX_M_ID
#elif defined(VTX_S)
#define VTX_ID VTX_S_ID
#elif defined(VTX_R)
#define VTX_ID VTX_R_ID
#elif defined(VTX_WL)
#define VTX_ID VTX_WL_ID
#elif defined(VTX_L)
#define VTX_ID VTX_L_ID
#endif

#endif /* __CONFIG_H_ */
