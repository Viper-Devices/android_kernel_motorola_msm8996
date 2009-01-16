/* arch/arm/plat-s3c64xx/include/plat/regs-clock.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C64XX clock register definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __PLAT_REGS_CLOCK_H
#define __PLAT_REGS_CLOCK_H __FILE__

#define S3C_CLKREG(x)		(S3C_VA_SYS + (x))

#define S3C_APLL_LOCK		S3C_CLKREG(0x00)
#define S3C_MPLL_LOCK		S3C_CLKREG(0x04)
#define S3C_EPLL_LOCK		S3C_CLKREG(0x08)
#define S3C_APLL_CON		S3C_CLKREG(0x0C)
#define S3C_MPLL_CON		S3C_CLKREG(0x10)
#define S3C_EPLL_CON0		S3C_CLKREG(0x14)
#define S3C_EPLL_CON1		S3C_CLKREG(0x18)
#define S3C_CLK_SRC		S3C_CLKREG(0x1C)
#define S3C_CLK_DIV0		S3C_CLKREG(0x20)
#define S3C_CLK_DIV1		S3C_CLKREG(0x24)
#define S3C_CLK_DIV2		S3C_CLKREG(0x28)
#define S3C_CLK_OUT		S3C_CLKREG(0x2C)
#define S3C_HCLK_GATE		S3C_CLKREG(0x30)
#define S3C_PCLK_GATE		S3C_CLKREG(0x34)
#define S3C_SCLK_GATE		S3C_CLKREG(0x38)

/* CLKDIV0 */
#define S3C6400_CLKDIV0_MFC_MASK	(0xf << 28)
#define S3C6400_CLKDIV0_MFC_SHIFT	(28)
#define S3C6400_CLKDIV0_JPEG_MASK	(0xf << 24)
#define S3C6400_CLKDIV0_JPEG_SHIFT	(24)
#define S3C6400_CLKDIV0_CAM_MASK	(0xf << 20)
#define S3C6400_CLKDIV0_CAM_SHIFT	(20)
#define S3C6400_CLKDIV0_SECURITY_MASK	(0x3 << 18)
#define S3C6400_CLKDIV0_SECURITY_SHIFT	(18)
#define S3C6400_CLKDIV0_PCLK_MASK	(0xf << 12)
#define S3C6400_CLKDIV0_PCLK_SHIFT	(12)
#define S3C6400_CLKDIV0_HCLK2_MASK	(0x7 << 9)
#define S3C6400_CLKDIV0_HCLK2_SHIFT	(9)
#define S3C6400_CLKDIV0_HCLK_MASK	(0x1 << 8)
#define S3C6400_CLKDIV0_HCLK_SHIFT	(8)
#define S3C6400_CLKDIV0_MPLL_MASK	(0x1 << 4)
#define S3C6400_CLKDIV0_MPLL_SHIFT	(4)
#define S3C6400_CLKDIV0_ARM_MASK	(0x3 << 0)
#define S3C6410_CLKDIV0_ARM_MASK	(0x7 << 0)
#define S3C6400_CLKDIV0_ARM_SHIFT	(0)

/* CLKDIV1 */
#define S3C6410_CLKDIV1_FIMC_MASK	(0xf << 24)
#define S3C6410_CLKDIV1_FIMC_SHIFT	(24)
#define S3C6400_CLKDIV1_UHOST_MASK	(0xf << 20)
#define S3C6400_CLKDIV1_UHOST_SHIFT	(20)
#define S3C6400_CLKDIV1_SCALER_MASK	(0xf << 16)
#define S3C6400_CLKDIV1_SCALER_SHIFT	(16)
#define S3C6400_CLKDIV1_LCD_MASK	(0xf << 12)
#define S3C6400_CLKDIV1_LCD_SHIFT	(12)
#define S3C6400_CLKDIV1_MMC2_MASK	(0xf << 8)
#define S3C6400_CLKDIV1_MMC2_SHIFT	(8)
#define S3C6400_CLKDIV1_MMC1_MASK	(0xf << 4)
#define S3C6400_CLKDIV1_MMC1_SHIFT	(4)
#define S3C6400_CLKDIV1_MMC0_MASK	(0xf << 0)
#define S3C6400_CLKDIV1_MMC0_SHIFT	(0)

/* CLKDIV2 */
#define S3C6410_CLKDIV2_AUDIO2_MASK	(0xf << 24)
#define S3C6410_CLKDIV2_AUDIO2_SHIFT	(24)
#define S3C6400_CLKDIV2_IRDA_MASK	(0xf << 20)
#define S3C6400_CLKDIV2_IRDA_SHIFT	(20)
#define S3C6400_CLKDIV2_UART_MASK	(0xf << 16)
#define S3C6400_CLKDIV2_UART_SHIFT	(16)
#define S3C6400_CLKDIV2_AUDIO1_MASK	(0xf << 12)
#define S3C6400_CLKDIV2_AUDIO1_SHIFT	(12)
#define S3C6400_CLKDIV2_AUDIO0_MASK	(0xf << 8)
#define S3C6400_CLKDIV2_AUDIO0_SHIFT	(8)
#define S3C6400_CLKDIV2_SPI1_MASK	(0xf << 4)
#define S3C6400_CLKDIV2_SPI1_SHIFT	(4)
#define S3C6400_CLKDIV2_SPI0_MASK	(0xf << 0)
#define S3C6400_CLKDIV2_SPI0_SHIFT	(0)

/* HCLK GATE Registers */
#define S3C_CLKCON_HCLK_BUS	(1<<30)
#define S3C_CLKCON_HCLK_SECUR	(1<<29)
#define S3C_CLKCON_HCLK_SDMA1	(1<<28)
#define S3C_CLKCON_HCLK_SDMA2	(1<<27)
#define S3C_CLKCON_HCLK_UHOST	(1<<26)
#define S3C_CLKCON_HCLK_IROM	(1<<25)
#define S3C_CLKCON_HCLK_DDR1	(1<<24)
#define S3C_CLKCON_HCLK_DDR0	(1<<23)
#define S3C_CLKCON_HCLK_MEM1	(1<<22)
#define S3C_CLKCON_HCLK_MEM0	(1<<21)
#define S3C_CLKCON_HCLK_USB	(1<<20)
#define S3C_CLKCON_HCLK_HSMMC2	(1<<19)
#define S3C_CLKCON_HCLK_HSMMC1	(1<<18)
#define S3C_CLKCON_HCLK_HSMMC0	(1<<17)
#define S3C_CLKCON_HCLK_MDP	(1<<16)
#define S3C_CLKCON_HCLK_DHOST	(1<<15)
#define S3C_CLKCON_HCLK_IHOST	(1<<14)
#define S3C_CLKCON_HCLK_DMA1	(1<<13)
#define S3C_CLKCON_HCLK_DMA0	(1<<12)
#define S3C_CLKCON_HCLK_JPEG	(1<<11)
#define S3C_CLKCON_HCLK_CAMIF	(1<<10)
#define S3C_CLKCON_HCLK_SCALER	(1<<9)
#define S3C_CLKCON_HCLK_2D	(1<<8)
#define S3C_CLKCON_HCLK_TV	(1<<7)
#define S3C_CLKCON_HCLK_POST0	(1<<5)
#define S3C_CLKCON_HCLK_ROT	(1<<4)
#define S3C_CLKCON_HCLK_LCD	(1<<3)
#define S3C_CLKCON_HCLK_TZIC	(1<<2)
#define S3C_CLKCON_HCLK_INTC	(1<<1)
#define S3C_CLKCON_HCLK_MFC	(1<<0)

/* PCLK GATE Registers */
#define S3C6410_CLKCON_PCLK_I2C1	(1<<27)
#define S3C6410_CLKCON_PCLK_IIS2	(1<<26)
#define S3C_CLKCON_PCLK_SKEY		(1<<24)
#define S3C_CLKCON_PCLK_CHIPID		(1<<23)
#define S3C_CLKCON_PCLK_SPI1		(1<<22)
#define S3C_CLKCON_PCLK_SPI0		(1<<21)
#define S3C_CLKCON_PCLK_HSIRX		(1<<20)
#define S3C_CLKCON_PCLK_HSITX		(1<<19)
#define S3C_CLKCON_PCLK_GPIO		(1<<18)
#define S3C_CLKCON_PCLK_IIC		(1<<17)
#define S3C_CLKCON_PCLK_IIS1		(1<<16)
#define S3C_CLKCON_PCLK_IIS0		(1<<15)
#define S3C_CLKCON_PCLK_AC97		(1<<14)
#define S3C_CLKCON_PCLK_TZPC		(1<<13)
#define S3C_CLKCON_PCLK_TSADC		(1<<12)
#define S3C_CLKCON_PCLK_KEYPAD		(1<<11)
#define S3C_CLKCON_PCLK_IRDA		(1<<10)
#define S3C_CLKCON_PCLK_PCM1		(1<<9)
#define S3C_CLKCON_PCLK_PCM0		(1<<8)
#define S3C_CLKCON_PCLK_PWM		(1<<7)
#define S3C_CLKCON_PCLK_RTC		(1<<6)
#define S3C_CLKCON_PCLK_WDT		(1<<5)
#define S3C_CLKCON_PCLK_UART3		(1<<4)
#define S3C_CLKCON_PCLK_UART2		(1<<3)
#define S3C_CLKCON_PCLK_UART1		(1<<2)
#define S3C_CLKCON_PCLK_UART0		(1<<1)
#define S3C_CLKCON_PCLK_MFC		(1<<0)

/* SCLK GATE Registers */
#define S3C_CLKCON_SCLK_UHOST		(1<<30)
#define S3C_CLKCON_SCLK_MMC2_48		(1<<29)
#define S3C_CLKCON_SCLK_MMC1_48		(1<<28)
#define S3C_CLKCON_SCLK_MMC0_48		(1<<27)
#define S3C_CLKCON_SCLK_MMC2		(1<<26)
#define S3C_CLKCON_SCLK_MMC1		(1<<25)
#define S3C_CLKCON_SCLK_MMC0		(1<<24)
#define S3C_CLKCON_SCLK_SPI1_48 	(1<<23)
#define S3C_CLKCON_SCLK_SPI0_48 	(1<<22)
#define S3C_CLKCON_SCLK_SPI1		(1<<21)
#define S3C_CLKCON_SCLK_SPI0		(1<<20)
#define S3C_CLKCON_SCLK_DAC27		(1<<19)
#define S3C_CLKCON_SCLK_TV27		(1<<18)
#define S3C_CLKCON_SCLK_SCALER27	(1<<17)
#define S3C_CLKCON_SCLK_SCALER		(1<<16)
#define S3C_CLKCON_SCLK_LCD27		(1<<15)
#define S3C_CLKCON_SCLK_LCD		(1<<14)
#define S3C6400_CLKCON_SCLK_POST1_27	(1<<13)
#define S3C6410_CLKCON_FIMC		(1<<13)
#define S3C_CLKCON_SCLK_POST0_27	(1<<12)
#define S3C6400_CLKCON_SCLK_POST1	(1<<11)
#define S3C6410_CLKCON_SCLK_AUDIO2	(1<<11)
#define S3C_CLKCON_SCLK_POST0		(1<<10)
#define S3C_CLKCON_SCLK_AUDIO1		(1<<9)
#define S3C_CLKCON_SCLK_AUDIO0		(1<<8)
#define S3C_CLKCON_SCLK_SECUR		(1<<7)
#define S3C_CLKCON_SCLK_IRDA		(1<<6)
#define S3C_CLKCON_SCLK_UART		(1<<5)
#define S3C_CLKCON_SCLK_ONENAND 	(1<<4)
#define S3C_CLKCON_SCLK_MFC		(1<<3)
#define S3C_CLKCON_SCLK_CAM		(1<<2)
#define S3C_CLKCON_SCLK_JPEG		(1<<1)

/* CLKSRC */

#define S3C6400_CLKSRC_APLL_MOUT	(1 << 0)
#define S3C6400_CLKSRC_MPLL_MOUT	(1 << 1)
#define S3C6400_CLKSRC_EPLL_MOUT	(1 << 2)
#define S3C6400_CLKSRC_APLL_MOUT_SHIFT	(0)
#define S3C6400_CLKSRC_MPLL_MOUT_SHIFT	(1)
#define S3C6400_CLKSRC_EPLL_MOUT_SHIFT	(2)
#define S3C6400_CLKSRC_MFC		(1 << 4)

#define S3C6410_CLKSRC_TV27_MASK	(0x1 << 31)
#define S3C6410_CLKSRC_TV27_SHIFT	(31)
#define S3C6410_CLKSRC_DAC27_MASK	(0x1 << 30)
#define S3C6410_CLKSRC_DAC27_SHIFT	(30)
#define S3C6400_CLKSRC_SCALER_MASK	(0x3 << 28)
#define S3C6400_CLKSRC_SCALER_SHIFT	(28)
#define S3C6400_CLKSRC_LCD_MASK		(0x3 << 26)
#define S3C6400_CLKSRC_LCD_SHIFT	(26)
#define S3C6400_CLKSRC_IRDA_MASK	(0x3 << 24)
#define S3C6400_CLKSRC_IRDA_SHIFT	(24)
#define S3C6400_CLKSRC_MMC2_MASK	(0x3 << 22)
#define S3C6400_CLKSRC_MMC2_SHIFT	(22)
#define S3C6400_CLKSRC_MMC1_MASK	(0x3 << 20)
#define S3C6400_CLKSRC_MMC1_SHIFT	(20)
#define S3C6400_CLKSRC_MMC0_MASK	(0x3 << 18)
#define S3C6400_CLKSRC_MMC0_SHIFT	(18)
#define S3C6400_CLKSRC_SPI1_MASK	(0x3 << 16)
#define S3C6400_CLKSRC_SPI1_SHIFT	(16)
#define S3C6400_CLKSRC_SPI0_MASK	(0x3 << 14)
#define S3C6400_CLKSRC_SPI0_SHIFT	(14)
#define S3C6400_CLKSRC_UART_MASK	(0x1 << 13)
#define S3C6400_CLKSRC_UART_SHIFT	(13)
#define S3C6400_CLKSRC_AUDIO1_MASK	(0x7 << 10)
#define S3C6400_CLKSRC_AUDIO1_SHIFT	(10)
#define S3C6400_CLKSRC_AUDIO0_MASK	(0x7 << 7)
#define S3C6400_CLKSRC_AUDIO0_SHIFT	(7)
#define S3C6400_CLKSRC_UHOST_MASK	(0x3 << 5)
#define S3C6400_CLKSRC_UHOST_SHIFT	(5)


#endif /* _PLAT_REGS_CLOCK_H */
