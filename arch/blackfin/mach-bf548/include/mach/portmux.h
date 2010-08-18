/*
 * Copyright 2007-2009 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#ifndef _MACH_PORTMUX_H_
#define _MACH_PORTMUX_H_

#define MAX_RESOURCES	MAX_BLACKFIN_GPIOS

#define P_SPORT2_TFS	(P_DEFINED | P_IDENT(GPIO_PA0) | P_FUNCT(0))
#define P_SPORT2_DTSEC	(P_DEFINED | P_IDENT(GPIO_PA1) | P_FUNCT(0))
#define P_SPORT2_DTPRI	(P_DEFINED | P_IDENT(GPIO_PA2) | P_FUNCT(0))
#define P_SPORT2_TSCLK	(P_DEFINED | P_IDENT(GPIO_PA3) | P_FUNCT(0))
#define P_SPORT2_RFS	(P_DEFINED | P_IDENT(GPIO_PA4) | P_FUNCT(0))
#define P_SPORT2_DRSEC	(P_DEFINED | P_IDENT(GPIO_PA5) | P_FUNCT(0))
#define P_SPORT2_DRPRI	(P_DEFINED | P_IDENT(GPIO_PA6) | P_FUNCT(0))
#define P_SPORT2_RSCLK	(P_DEFINED | P_IDENT(GPIO_PA7) | P_FUNCT(0))
#define P_SPORT3_TFS	(P_DEFINED | P_IDENT(GPIO_PA8) | P_FUNCT(0))
#define P_SPORT3_DTSEC	(P_DEFINED | P_IDENT(GPIO_PA9) | P_FUNCT(0))
#define P_SPORT3_DTPRI	(P_DEFINED | P_IDENT(GPIO_PA10) | P_FUNCT(0))
#define P_SPORT3_TSCLK	(P_DEFINED | P_IDENT(GPIO_PA11) | P_FUNCT(0))
#define P_SPORT3_RFS	(P_DEFINED | P_IDENT(GPIO_PA12) | P_FUNCT(0))
#define P_SPORT3_DRSEC	(P_DEFINED | P_IDENT(GPIO_PA13) | P_FUNCT(0))
#define P_SPORT3_DRPRI	(P_DEFINED | P_IDENT(GPIO_PA14) | P_FUNCT(0))
#define P_SPORT3_RSCLK	(P_DEFINED | P_IDENT(GPIO_PA15) | P_FUNCT(0))
#define P_TMR4	(P_DEFINED | P_IDENT(GPIO_PA1) | P_FUNCT(1))
#define P_TMR5	(P_DEFINED | P_IDENT(GPIO_PA5) | P_FUNCT(1))
#define P_TMR6	(P_DEFINED | P_IDENT(GPIO_PA9) | P_FUNCT(1))
#define P_TMR7	(P_DEFINED | P_IDENT(GPIO_PA13) | P_FUNCT(1))

#define P_TWI1_SCL	(P_DEFINED | P_IDENT(GPIO_PB0) | P_FUNCT(0))
#define P_TWI1_SDA	(P_DEFINED | P_IDENT(GPIO_PB1) | P_FUNCT(0))
#define P_UART3_RTS	(P_DEFINED | P_IDENT(GPIO_PB2) | P_FUNCT(0))
#define P_UART3_CTS	(P_DEFINED | P_IDENT(GPIO_PB3) | P_FUNCT(0))
#define P_UART2_TX	(P_DEFINED | P_IDENT(GPIO_PB4) | P_FUNCT(0))
#define P_UART2_RX	(P_DEFINED | P_IDENT(GPIO_PB5) | P_FUNCT(0))
#define P_UART3_TX	(P_DEFINED | P_IDENT(GPIO_PB6) | P_FUNCT(0))
#define P_UART3_RX	(P_DEFINED | P_IDENT(GPIO_PB7) | P_FUNCT(0))
#define P_SPI2_SS	(P_DEFINED | P_IDENT(GPIO_PB8) | P_FUNCT(0))
#define P_SPI2_SSEL1	(P_DEFINED | P_IDENT(GPIO_PB9) | P_FUNCT(0))
#define P_SPI2_SSEL2	(P_DEFINED | P_IDENT(GPIO_PB10) | P_FUNCT(0))
#define P_SPI2_SSEL3	(P_DEFINED | P_IDENT(GPIO_PB11) | P_FUNCT(0))
#define P_SPI2_SCK	(P_DEFINED | P_IDENT(GPIO_PB12) | P_FUNCT(0))
#define P_SPI2_MOSI	(P_DEFINED | P_IDENT(GPIO_PB13) | P_FUNCT(0))
#define P_SPI2_MISO	(P_DEFINED | P_IDENT(GPIO_PB14) | P_FUNCT(0))
#define P_TMR0	(P_DEFINED | P_IDENT(GPIO_PB8) | P_FUNCT(1))
#define P_TMR1	(P_DEFINED | P_IDENT(GPIO_PB9) | P_FUNCT(1))
#define P_TMR2	(P_DEFINED | P_IDENT(GPIO_PB10) | P_FUNCT(1))
#define P_TMR3	(P_DEFINED | P_IDENT(GPIO_PB11) | P_FUNCT(1))

#define P_SPORT0_TFS	(P_DEFINED | P_IDENT(GPIO_PC0) | P_FUNCT(0))
#define P_SPORT0_DTSEC	(P_DEFINED | P_IDENT(GPIO_PC1) | P_FUNCT(0))
#define P_SPORT0_DTPRI	(P_DEFINED | P_IDENT(GPIO_PC2) | P_FUNCT(0))
#define P_SPORT0_TSCLK	(P_DEFINED | P_IDENT(GPIO_PC3) | P_FUNCT(0))
#define P_SPORT0_RFS	(P_DEFINED | P_IDENT(GPIO_PC4) | P_FUNCT(0))
#define P_SPORT0_DRSEC	(P_DEFINED | P_IDENT(GPIO_PC5) | P_FUNCT(0))
#define P_SPORT0_DRPRI	(P_DEFINED | P_IDENT(GPIO_PC6) | P_FUNCT(0))
#define P_SPORT0_RSCLK	(P_DEFINED | P_IDENT(GPIO_PC7) | P_FUNCT(0))
#define P_SD_D0	(P_DEFINED | P_IDENT(GPIO_PC8) | P_FUNCT(0))
#define P_SD_D1	(P_DEFINED | P_IDENT(GPIO_PC9) | P_FUNCT(0))
#define P_SD_D2	(P_DEFINED | P_IDENT(GPIO_PC10) | P_FUNCT(0))
#define P_SD_D3	(P_DEFINED | P_IDENT(GPIO_PC11) | P_FUNCT(0))
#define P_SD_CLK	(P_DEFINED | P_IDENT(GPIO_PC12) | P_FUNCT(0))
#define P_SD_CMD	(P_DEFINED | P_IDENT(GPIO_PC13) | P_FUNCT(0))
#define P_MMCLK	(P_DEFINED | P_IDENT(GPIO_PC1) | P_FUNCT(1))
#define P_MBCLK	(P_DEFINED | P_IDENT(GPIO_PC5) | P_FUNCT(1))

#define P_PPI1_D0	(P_DEFINED | P_IDENT(GPIO_PD0) | P_FUNCT(0))
#define P_PPI1_D1	(P_DEFINED | P_IDENT(GPIO_PD1) | P_FUNCT(0))
#define P_PPI1_D2	(P_DEFINED | P_IDENT(GPIO_PD2) | P_FUNCT(0))
#define P_PPI1_D3	(P_DEFINED | P_IDENT(GPIO_PD3) | P_FUNCT(0))
#define P_PPI1_D4	(P_DEFINED | P_IDENT(GPIO_PD4) | P_FUNCT(0))
#define P_PPI1_D5	(P_DEFINED | P_IDENT(GPIO_PD5) | P_FUNCT(0))
#define P_PPI1_D6	(P_DEFINED | P_IDENT(GPIO_PD6) | P_FUNCT(0))
#define P_PPI1_D7	(P_DEFINED | P_IDENT(GPIO_PD7) | P_FUNCT(0))
#define P_PPI1_D8	(P_DEFINED | P_IDENT(GPIO_PD8) | P_FUNCT(0))
#define P_PPI1_D9	(P_DEFINED | P_IDENT(GPIO_PD9) | P_FUNCT(0))
#define P_PPI1_D10	(P_DEFINED | P_IDENT(GPIO_PD10) | P_FUNCT(0))
#define P_PPI1_D11	(P_DEFINED | P_IDENT(GPIO_PD11) | P_FUNCT(0))
#define P_PPI1_D12	(P_DEFINED | P_IDENT(GPIO_PD12) | P_FUNCT(0))
#define P_PPI1_D13	(P_DEFINED | P_IDENT(GPIO_PD13) | P_FUNCT(0))
#define P_PPI1_D14	(P_DEFINED | P_IDENT(GPIO_PD14) | P_FUNCT(0))
#define P_PPI1_D15	(P_DEFINED | P_IDENT(GPIO_PD15) | P_FUNCT(0))

#define P_HOST_D8	(P_DEFINED | P_IDENT(GPIO_PD0) | P_FUNCT(1))
#define P_HOST_D9	(P_DEFINED | P_IDENT(GPIO_PD1) | P_FUNCT(1))
#define P_HOST_D10	(P_DEFINED | P_IDENT(GPIO_PD2) | P_FUNCT(1))
#define P_HOST_D11	(P_DEFINED | P_IDENT(GPIO_PD3) | P_FUNCT(1))
#define P_HOST_D12	(P_DEFINED | P_IDENT(GPIO_PD4) | P_FUNCT(1))
#define P_HOST_D13	(P_DEFINED | P_IDENT(GPIO_PD5) | P_FUNCT(1))
#define P_HOST_D14	(P_DEFINED | P_IDENT(GPIO_PD6) | P_FUNCT(1))
#define P_HOST_D15	(P_DEFINED | P_IDENT(GPIO_PD7) | P_FUNCT(1))
#define P_HOST_D0	(P_DEFINED | P_IDENT(GPIO_PD8) | P_FUNCT(1))
#define P_HOST_D1	(P_DEFINED | P_IDENT(GPIO_PD9) | P_FUNCT(1))
#define P_HOST_D2	(P_DEFINED | P_IDENT(GPIO_PD10) | P_FUNCT(1))
#define P_HOST_D3	(P_DEFINED | P_IDENT(GPIO_PD11) | P_FUNCT(1))
#define P_HOST_D4	(P_DEFINED | P_IDENT(GPIO_PD12) | P_FUNCT(1))
#define P_HOST_D5	(P_DEFINED | P_IDENT(GPIO_PD13) | P_FUNCT(1))
#define P_HOST_D6	(P_DEFINED | P_IDENT(GPIO_PD14) | P_FUNCT(1))
#define P_HOST_D7	(P_DEFINED | P_IDENT(GPIO_PD15) | P_FUNCT(1))
#define P_SPORT1_TFS	(P_DEFINED | P_IDENT(GPIO_PD0) | P_FUNCT(2))
#define P_SPORT1_DTSEC	(P_DEFINED | P_IDENT(GPIO_PD1) | P_FUNCT(2))
#define P_SPORT1_DTPRI	(P_DEFINED | P_IDENT(GPIO_PD2) | P_FUNCT(2))
#define P_SPORT1_TSCLK	(P_DEFINED | P_IDENT(GPIO_PD3) | P_FUNCT(2))
#define P_SPORT1_RFS	(P_DEFINED | P_IDENT(GPIO_PD4) | P_FUNCT(2))
#define P_SPORT1_DRSEC	(P_DEFINED | P_IDENT(GPIO_PD5) | P_FUNCT(2))
#define P_SPORT1_DRPRI	(P_DEFINED | P_IDENT(GPIO_PD6) | P_FUNCT(2))
#define P_SPORT1_RSCLK	(P_DEFINED | P_IDENT(GPIO_PD7) | P_FUNCT(2))
#define P_PPI2_D0	(P_DEFINED | P_IDENT(GPIO_PD8) | P_FUNCT(2))
#define P_PPI2_D1	(P_DEFINED | P_IDENT(GPIO_PD9) | P_FUNCT(2))
#define P_PPI2_D2	(P_DEFINED | P_IDENT(GPIO_PD10) | P_FUNCT(2))
#define P_PPI2_D3	(P_DEFINED | P_IDENT(GPIO_PD11) | P_FUNCT(2))
#define P_PPI2_D4	(P_DEFINED | P_IDENT(GPIO_PD12) | P_FUNCT(2))
#define P_PPI2_D5	(P_DEFINED | P_IDENT(GPIO_PD13) | P_FUNCT(2))
#define P_PPI2_D6	(P_DEFINED | P_IDENT(GPIO_PD14) | P_FUNCT(2))
#define P_PPI2_D7	(P_DEFINED | P_IDENT(GPIO_PD15) | P_FUNCT(2))
#define P_PPI0_D18	(P_DEFINED | P_IDENT(GPIO_PD0) | P_FUNCT(3))
#define P_PPI0_D19	(P_DEFINED | P_IDENT(GPIO_PD1) | P_FUNCT(3))
#define P_PPI0_D20	(P_DEFINED | P_IDENT(GPIO_PD2) | P_FUNCT(3))
#define P_PPI0_D21	(P_DEFINED | P_IDENT(GPIO_PD3) | P_FUNCT(3))
#define P_PPI0_D22	(P_DEFINED | P_IDENT(GPIO_PD4) | P_FUNCT(3))
#define P_PPI0_D23	(P_DEFINED | P_IDENT(GPIO_PD5) | P_FUNCT(3))
#define P_KEY_ROW0	(P_DEFINED | P_IDENT(GPIO_PD8) | P_FUNCT(3))
#define P_KEY_ROW1	(P_DEFINED | P_IDENT(GPIO_PD9) | P_FUNCT(3))
#define P_KEY_ROW2	(P_DEFINED | P_IDENT(GPIO_PD10) | P_FUNCT(3))
#define P_KEY_ROW3	(P_DEFINED | P_IDENT(GPIO_PD11) | P_FUNCT(3))
#define P_KEY_COL0	(P_DEFINED | P_IDENT(GPIO_PD12) | P_FUNCT(3))
#define P_KEY_COL1	(P_DEFINED | P_IDENT(GPIO_PD13) | P_FUNCT(3))
#define P_KEY_COL2	(P_DEFINED | P_IDENT(GPIO_PD14) | P_FUNCT(3))
#define P_KEY_COL3	(P_DEFINED | P_IDENT(GPIO_PD15) | P_FUNCT(3))

#define GPIO_DEFAULT_BOOT_SPI_CS GPIO_PE4
#define P_DEFAULT_BOOT_SPI_CS P_SPI0_SSEL1
#define P_SPI0_SCK	(P_DEFINED | P_IDENT(GPIO_PE0) | P_FUNCT(0))
#define P_SPI0_MISO	(P_DEFINED | P_IDENT(GPIO_PE1) | P_FUNCT(0))
#define P_SPI0_MOSI	(P_DEFINED | P_IDENT(GPIO_PE2) | P_FUNCT(0))
#define P_SPI0_SS	(P_DEFINED | P_IDENT(GPIO_PE3) | P_FUNCT(0))
#define P_SPI0_SSEL1	(P_DEFINED | P_IDENT(GPIO_PE4) | P_FUNCT(0))
#define P_SPI0_SSEL2	(P_DEFINED | P_IDENT(GPIO_PE5) | P_FUNCT(0))
#define P_SPI0_SSEL3	(P_DEFINED | P_IDENT(GPIO_PE6) | P_FUNCT(0))
#define P_UART0_TX	(P_DEFINED | P_IDENT(GPIO_PE7) | P_FUNCT(0))
#define P_UART0_RX	(P_DEFINED | P_IDENT(GPIO_PE8) | P_FUNCT(0))
#define P_UART1_RTS	(P_DEFINED | P_IDENT(GPIO_PE9) | P_FUNCT(0))
#define P_UART1_CTS	(P_DEFINED | P_IDENT(GPIO_PE10) | P_FUNCT(0))
#define P_PPI1_CLK	(P_DEFINED | P_IDENT(GPIO_PE11) | P_FUNCT(0))
#define P_PPI1_FS1	(P_DEFINED | P_IDENT(GPIO_PE12) | P_FUNCT(0))
#define P_PPI1_FS2	(P_DEFINED | P_IDENT(GPIO_PE13) | P_FUNCT(0))
#define P_TWI0_SCL	(P_DEFINED | P_IDENT(GPIO_PE14) | P_FUNCT(0))
#define P_TWI0_SDA	(P_DEFINED | P_IDENT(GPIO_PE15) | P_FUNCT(0))
#define P_KEY_COL7	(P_DEFINED | P_IDENT(GPIO_PE0) | P_FUNCT(1))
#define P_KEY_ROW6	(P_DEFINED | P_IDENT(GPIO_PE1) | P_FUNCT(1))
#define P_KEY_COL6	(P_DEFINED | P_IDENT(GPIO_PE2) | P_FUNCT(1))
#define P_KEY_ROW5	(P_DEFINED | P_IDENT(GPIO_PE3) | P_FUNCT(1))
#define P_KEY_COL5	(P_DEFINED | P_IDENT(GPIO_PE4) | P_FUNCT(1))
#define P_KEY_ROW4	(P_DEFINED | P_IDENT(GPIO_PE5) | P_FUNCT(1))
#define P_KEY_COL4	(P_DEFINED | P_IDENT(GPIO_PE6) | P_FUNCT(1))
#define P_KEY_ROW7	(P_DEFINED | P_IDENT(GPIO_PE7) | P_FUNCT(1))

#define P_PPI0_D0	(P_DEFINED | P_IDENT(GPIO_PF0) | P_FUNCT(0))
#define P_PPI0_D1	(P_DEFINED | P_IDENT(GPIO_PF1) | P_FUNCT(0))
#define P_PPI0_D2	(P_DEFINED | P_IDENT(GPIO_PF2) | P_FUNCT(0))
#define P_PPI0_D3	(P_DEFINED | P_IDENT(GPIO_PF3) | P_FUNCT(0))
#define P_PPI0_D4	(P_DEFINED | P_IDENT(GPIO_PF4) | P_FUNCT(0))
#define P_PPI0_D5	(P_DEFINED | P_IDENT(GPIO_PF5) | P_FUNCT(0))
#define P_PPI0_D6	(P_DEFINED | P_IDENT(GPIO_PF6) | P_FUNCT(0))
#define P_PPI0_D7	(P_DEFINED | P_IDENT(GPIO_PF7) | P_FUNCT(0))
#define P_PPI0_D8	(P_DEFINED | P_IDENT(GPIO_PF8) | P_FUNCT(0))
#define P_PPI0_D9	(P_DEFINED | P_IDENT(GPIO_PF9) | P_FUNCT(0))
#define P_PPI0_D10	(P_DEFINED | P_IDENT(GPIO_PF10) | P_FUNCT(0))
#define P_PPI0_D11	(P_DEFINED | P_IDENT(GPIO_PF11) | P_FUNCT(0))
#define P_PPI0_D12	(P_DEFINED | P_IDENT(GPIO_PF12) | P_FUNCT(0))
#define P_PPI0_D13	(P_DEFINED | P_IDENT(GPIO_PF13) | P_FUNCT(0))
#define P_PPI0_D14	(P_DEFINED | P_IDENT(GPIO_PF14) | P_FUNCT(0))
#define P_PPI0_D15	(P_DEFINED | P_IDENT(GPIO_PF15) | P_FUNCT(0))

#ifdef CONFIG_BF548_ATAPI_ALTERNATIVE_PORT
# define P_ATAPI_D0A	(P_DEFINED | P_IDENT(GPIO_PF0) | P_FUNCT(1))
# define P_ATAPI_D1A	(P_DEFINED | P_IDENT(GPIO_PF1) | P_FUNCT(1))
# define P_ATAPI_D2A	(P_DEFINED | P_IDENT(GPIO_PF2) | P_FUNCT(1))
# define P_ATAPI_D3A	(P_DEFINED | P_IDENT(GPIO_PF3) | P_FUNCT(1))
# define P_ATAPI_D4A	(P_DEFINED | P_IDENT(GPIO_PF4) | P_FUNCT(1))
# define P_ATAPI_D5A	(P_DEFINED | P_IDENT(GPIO_PF5) | P_FUNCT(1))
# define P_ATAPI_D6A	(P_DEFINED | P_IDENT(GPIO_PF6) | P_FUNCT(1))
# define P_ATAPI_D7A	(P_DEFINED | P_IDENT(GPIO_PF7) | P_FUNCT(1))
# define P_ATAPI_D8A	(P_DEFINED | P_IDENT(GPIO_PF8) | P_FUNCT(1))
# define P_ATAPI_D9A	(P_DEFINED | P_IDENT(GPIO_PF9) | P_FUNCT(1))
# define P_ATAPI_D10A	(P_DEFINED | P_IDENT(GPIO_PF10) | P_FUNCT(1))
# define P_ATAPI_D11A	(P_DEFINED | P_IDENT(GPIO_PF11) | P_FUNCT(1))
# define P_ATAPI_D12A	(P_DEFINED | P_IDENT(GPIO_PF12) | P_FUNCT(1))
# define P_ATAPI_D13A	(P_DEFINED | P_IDENT(GPIO_PF13) | P_FUNCT(1))
# define P_ATAPI_D14A	(P_DEFINED | P_IDENT(GPIO_PF14) | P_FUNCT(1))
# define P_ATAPI_D15A	(P_DEFINED | P_IDENT(GPIO_PF15) | P_FUNCT(1))
#else
# define P_ATAPI_D0A	(P_DONTCARE)
# define P_ATAPI_D1A	(P_DONTCARE)
# define P_ATAPI_D2A	(P_DONTCARE)
# define P_ATAPI_D3A	(P_DONTCARE)
# define P_ATAPI_D4A	(P_DONTCARE)
# define P_ATAPI_D5A	(P_DONTCARE)
# define P_ATAPI_D6A	(P_DONTCARE)
# define P_ATAPI_D7A	(P_DONTCARE)
# define P_ATAPI_D8A	(P_DONTCARE)
# define P_ATAPI_D9A	(P_DONTCARE)
# define P_ATAPI_D10A	(P_DONTCARE)
# define P_ATAPI_D11A	(P_DONTCARE)
# define P_ATAPI_D12A	(P_DONTCARE)
# define P_ATAPI_D13A	(P_DONTCARE)
# define P_ATAPI_D14A	(P_DONTCARE)
# define P_ATAPI_D15A	(P_DONTCARE)
#endif

#define P_PPI0_CLK	(P_DEFINED | P_IDENT(GPIO_PG0) | P_FUNCT(0))
#define P_PPI0_FS1	(P_DEFINED | P_IDENT(GPIO_PG1) | P_FUNCT(0))
#define P_PPI0_FS2	(P_DEFINED | P_IDENT(GPIO_PG2) | P_FUNCT(0))
#define P_PPI0_D16	(P_DEFINED | P_IDENT(GPIO_PG3) | P_FUNCT(0))
#define P_PPI0_D17	(P_DEFINED | P_IDENT(GPIO_PG4) | P_FUNCT(0))
#define P_SPI1_SSEL1	(P_DEFINED | P_IDENT(GPIO_PG5) | P_FUNCT(0))
#define P_SPI1_SSEL2	(P_DEFINED | P_IDENT(GPIO_PG6) | P_FUNCT(0))
#define P_SPI1_SSEL3	(P_DEFINED | P_IDENT(GPIO_PG7) | P_FUNCT(0))
#define P_SPI1_SCK	(P_DEFINED | P_IDENT(GPIO_PG8) | P_FUNCT(0))
#define P_SPI1_MISO	(P_DEFINED | P_IDENT(GPIO_PG9) | P_FUNCT(0))
#define P_SPI1_MOSI	(P_DEFINED | P_IDENT(GPIO_PG10) | P_FUNCT(0))
#define P_SPI1_SS	(P_DEFINED | P_IDENT(GPIO_PG11) | P_FUNCT(0))
#define P_CAN0_TX	(P_DEFINED | P_IDENT(GPIO_PG12) | P_FUNCT(0))
#define P_CAN0_RX	(P_DEFINED | P_IDENT(GPIO_PG13) | P_FUNCT(0))
#define P_CAN1_TX	(P_DEFINED | P_IDENT(GPIO_PG14) | P_FUNCT(0))
#define P_CAN1_RX	(P_DEFINED | P_IDENT(GPIO_PG15) | P_FUNCT(0))
#ifdef CONFIG_BF548_ATAPI_ALTERNATIVE_PORT
# define P_ATAPI_A0A	(P_DEFINED | P_IDENT(GPIO_PG2) | P_FUNCT(1))
# define P_ATAPI_A1A	(P_DEFINED | P_IDENT(GPIO_PG3) | P_FUNCT(1))
# define P_ATAPI_A2A	(P_DEFINED | P_IDENT(GPIO_PG4) | P_FUNCT(1))
#else
# define P_ATAPI_A0A	(P_DONTCARE)
# define P_ATAPI_A1A	(P_DONTCARE)
# define P_ATAPI_A2A	(P_DONTCARE)
#endif
#define P_HOST_CE	(P_DEFINED | P_IDENT(GPIO_PG5) | P_FUNCT(1))
#define P_HOST_RD	(P_DEFINED | P_IDENT(GPIO_PG6) | P_FUNCT(1))
#define P_HOST_WR	(P_DEFINED | P_IDENT(GPIO_PG7) | P_FUNCT(1))
#define P_MTXONB	(P_DEFINED | P_IDENT(GPIO_PG11) | P_FUNCT(1))
#define P_PPI2_FS2	(P_DEFINED | P_IDENT(GPIO_PG5) | P_FUNCT(2))
#define P_PPI2_FS1	(P_DEFINED | P_IDENT(GPIO_PG6) | P_FUNCT(2))
#define P_PPI2_CLK	(P_DEFINED | P_IDENT(GPIO_PG7) | P_FUNCT(2))
#define P_CNT_CZM	(P_DEFINED | P_IDENT(GPIO_PG5) | P_FUNCT(3))

#define P_UART1_TX	(P_DEFINED | P_IDENT(GPIO_PH0) | P_FUNCT(0))
#define P_UART1_RX	(P_DEFINED | P_IDENT(GPIO_PH1) | P_FUNCT(0))
#define P_ATAPI_RESET	(P_DEFINED | P_IDENT(GPIO_PH2) | P_FUNCT(0))
#define P_HOST_ADDR	(P_DEFINED | P_IDENT(GPIO_PH3) | P_FUNCT(0))
#define P_HOST_ACK	(P_DEFINED | P_IDENT(GPIO_PH4) | P_FUNCT(0))
#define P_MTX	(P_DEFINED | P_IDENT(GPIO_PH5) | P_FUNCT(0))
#define P_MRX	(P_DEFINED | P_IDENT(GPIO_PH6) | P_FUNCT(0))
#define P_MRXONB	(P_DEFINED | P_IDENT(GPIO_PH7) | P_FUNCT(0))
#define P_A4	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PH8) | P_FUNCT(0))
#define P_A5	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PH9) | P_FUNCT(0))
#define P_A6	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PH10) | P_FUNCT(0))
#define P_A7	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PH11) | P_FUNCT(0))
#define P_A8	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PH12) | P_FUNCT(0))
#define P_A9	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PH13) | P_FUNCT(0))
#define P_PPI1_FS3	(P_DEFINED | P_IDENT(GPIO_PH0) | P_FUNCT(1))
#define P_PPI2_FS3	(P_DEFINED | P_IDENT(GPIO_PH1) | P_FUNCT(1))
#define P_TMR8	(P_DEFINED | P_IDENT(GPIO_PH2) | P_FUNCT(1))
#define P_TMR9	(P_DEFINED | P_IDENT(GPIO_PH3) | P_FUNCT(1))
#define P_TMR10	(P_DEFINED | P_IDENT(GPIO_PH4) | P_FUNCT(1))
#define P_DMAR0	(P_DEFINED | P_IDENT(GPIO_PH5) | P_FUNCT(1))
#define P_DMAR1	(P_DEFINED | P_IDENT(GPIO_PH6) | P_FUNCT(1))
#define P_PPI0_FS3	(P_DEFINED | P_IDENT(GPIO_PH2) | P_FUNCT(2))
#define P_CNT_CDG	(P_DEFINED | P_IDENT(GPIO_PH3) | P_FUNCT(2))
#define P_CNT_CUD	(P_DEFINED | P_IDENT(GPIO_PH4) | P_FUNCT(2))

#define P_A10	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI0) | P_FUNCT(0))
#define P_A11	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI1) | P_FUNCT(0))
#define P_A12	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI2) | P_FUNCT(0))
#define P_A13	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI3) | P_FUNCT(0))
#define P_A14	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI4) | P_FUNCT(0))
#define P_A15	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI5) | P_FUNCT(0))
#define P_A16	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI6) | P_FUNCT(0))
#define P_A17	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI7) | P_FUNCT(0))
#define P_A18	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI8) | P_FUNCT(0))
#define P_A19	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI9) | P_FUNCT(0))
#define P_A20	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI10) | P_FUNCT(0))
#define P_A21	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI11) | P_FUNCT(0))
#define P_A22	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI12) | P_FUNCT(0))
#define P_A23	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI13) | P_FUNCT(0))
#define P_A24	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI14) | P_FUNCT(0))
#define P_A25	(P_MAYSHARE | P_DEFINED | P_IDENT(GPIO_PI15) | P_FUNCT(0))
#define P_NOR_CLK	(P_DEFINED | P_IDENT(GPIO_PI15) | P_FUNCT(1))

#define P_AMC_ARDY_NOR_WAIT	(P_DEFINED | P_IDENT(GPIO_PJ0) | P_FUNCT(0))
#define P_NAND_CE	(P_DEFINED | P_IDENT(GPIO_PJ1) | P_FUNCT(0))
#define P_NAND_RB	(P_DEFINED | P_IDENT(GPIO_PJ2) | P_FUNCT(0))
#define P_ATAPI_DIOR	(P_DEFINED | P_IDENT(GPIO_PJ3) | P_FUNCT(0))
#define P_ATAPI_DIOW	(P_DEFINED | P_IDENT(GPIO_PJ4) | P_FUNCT(0))
#define P_ATAPI_CS0	(P_DEFINED | P_IDENT(GPIO_PJ5) | P_FUNCT(0))
#define P_ATAPI_CS1	(P_DEFINED | P_IDENT(GPIO_PJ6) | P_FUNCT(0))
#define P_ATAPI_DMACK	(P_DEFINED | P_IDENT(GPIO_PJ7) | P_FUNCT(0))
#define P_ATAPI_DMARQ	(P_DEFINED | P_IDENT(GPIO_PJ8) | P_FUNCT(0))
#define P_ATAPI_INTRQ	(P_DEFINED | P_IDENT(GPIO_PJ9) | P_FUNCT(0))
#define P_ATAPI_IORDY	(P_DEFINED | P_IDENT(GPIO_PJ10) | P_FUNCT(0))
#define P_AMC_BR	(P_DEFINED | P_IDENT(GPIO_PJ11) | P_FUNCT(0))
#define P_AMC_BG	(P_DEFINED | P_IDENT(GPIO_PJ12) | P_FUNCT(0))
#define P_AMC_BGH	(P_DEFINED | P_IDENT(GPIO_PJ13) | P_FUNCT(0))


#define P_NAND_D0	(P_DONTCARE)
#define P_NAND_D1	(P_DONTCARE)
#define P_NAND_D2	(P_DONTCARE)
#define P_NAND_D3	(P_DONTCARE)
#define P_NAND_D4	(P_DONTCARE)
#define P_NAND_D5	(P_DONTCARE)
#define P_NAND_D6	(P_DONTCARE)
#define P_NAND_D7	(P_DONTCARE)
#define P_NAND_WE	(P_DONTCARE)
#define P_NAND_RE	(P_DONTCARE)
#define P_NAND_CLE	(P_DONTCARE)
#define P_NAND_ALE	(P_DONTCARE)

#endif /* _MACH_PORTMUX_H_ */
