#ifndef __CONFIG_H_
#define __CONFIG_H_

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

#endif /* __CONFIG_H_ */
