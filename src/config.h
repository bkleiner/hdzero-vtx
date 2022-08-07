#ifndef __CONFIG_H_
#define __CONFIG_H_

// this seems weird
#define MCU_IS_RX 0

#define DEBUG_MODE

#ifdef DEBUG_MODE
#define DEBUG_EEPROM
#endif

#define DEBUG_UART uart1

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
