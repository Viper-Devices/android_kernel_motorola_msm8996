/* linux/arch/arm/mach-exynos/include/mach/map.h
 *
 * Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS4 - Memory map definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_MAP_H
#define __ASM_ARCH_MAP_H __FILE__

#include <plat/map-base.h>

/*
 * EXYNOS4 UART offset is 0x10000 but the older S5P SoCs are 0x400.
 * So need to define it, and here is to avoid redefinition warning.
 */
#define S3C_UART_OFFSET			(0x10000)

#include <plat/map-s5p.h>

#define EXYNOS4_PA_SYSRAM0		0x02025000
#define EXYNOS4_PA_SYSRAM1		0x02020000
#define EXYNOS5_PA_SYSRAM		0x02020000

#define EXYNOS4_PA_FIMC0		0x11800000
#define EXYNOS4_PA_FIMC1		0x11810000
#define EXYNOS4_PA_FIMC2		0x11820000
#define EXYNOS4_PA_FIMC3		0x11830000

#define EXYNOS4_PA_JPEG			0x11840000

/* x = 0...1 */
#define EXYNOS4_PA_FIMC_LITE(x)		(0x12390000 + ((x) * 0x10000))

#define EXYNOS4_PA_G2D			0x12800000

#define EXYNOS4_PA_I2S0			0x03830000
#define EXYNOS4_PA_I2S1			0xE3100000
#define EXYNOS4_PA_I2S2			0xE2A00000

#define EXYNOS4_PA_PCM0			0x03840000
#define EXYNOS4_PA_PCM1			0x13980000
#define EXYNOS4_PA_PCM2			0x13990000

#define EXYNOS4_PA_SROM_BANK(x)		(0x04000000 + ((x) * 0x01000000))

#define EXYNOS4_PA_ONENAND		0x0C000000
#define EXYNOS4_PA_ONENAND_DMA		0x0C600000

#define EXYNOS_PA_CHIPID		0x10000000

#define EXYNOS4_PA_SYSCON		0x10010000
#define EXYNOS5_PA_SYSCON		0x10050100

#define EXYNOS4_PA_PMU			0x10020000
#define EXYNOS5_PA_PMU			0x10040000

#define EXYNOS4_PA_CMU			0x10030000
#define EXYNOS5_PA_CMU			0x10010000

#define EXYNOS4_PA_SYSTIMER		0x10050000
#define EXYNOS5_PA_SYSTIMER		0x101C0000

#define EXYNOS4_PA_WATCHDOG		0x10060000
#define EXYNOS5_PA_WATCHDOG		0x101D0000

#define EXYNOS4_PA_RTC			0x10070000

#define EXYNOS4_PA_KEYPAD		0x100A0000

#define EXYNOS4_PA_DMC0			0x10400000
#define EXYNOS4_PA_DMC1			0x10410000

#define EXYNOS4_PA_COMBINER		0x10440000
#define EXYNOS5_PA_COMBINER		0x10440000

#define EXYNOS4_PA_GIC_CPU		0x10480000
#define EXYNOS4_PA_GIC_DIST		0x10490000
#define EXYNOS5_PA_GIC_CPU		0x10482000
#define EXYNOS5_PA_GIC_DIST		0x10481000

#define EXYNOS4_PA_COREPERI		0x10500000
#define EXYNOS4_PA_TWD			0x10500600
#define EXYNOS4_PA_L2CC			0x10502000

#define EXYNOS4_PA_MDMA0		0x10810000
#define EXYNOS4_PA_MDMA1		0x12840000
#define EXYNOS4_PA_PDMA0		0x12680000
#define EXYNOS4_PA_PDMA1		0x12690000
#define EXYNOS5_PA_MDMA0		0x10800000
#define EXYNOS5_PA_MDMA1		0x11C10000
#define EXYNOS5_PA_PDMA0		0x121A0000
#define EXYNOS5_PA_PDMA1		0x121B0000

#define EXYNOS4_PA_SYSMMU_MDMA		0x10A40000
#define EXYNOS4_PA_SYSMMU_2D_ACP	0x10A40000
#define EXYNOS4_PA_SYSMMU_SSS		0x10A50000
#define EXYNOS4_PA_SYSMMU_FIMC0		0x11A20000
#define EXYNOS4_PA_SYSMMU_FIMC1		0x11A30000
#define EXYNOS4_PA_SYSMMU_FIMC2		0x11A40000
#define EXYNOS4_PA_SYSMMU_FIMC3		0x11A50000
#define EXYNOS4_PA_SYSMMU_JPEG		0x11A60000
#define EXYNOS4_PA_SYSMMU_FIMD0		0x11E20000
#define EXYNOS4_PA_SYSMMU_FIMD1		0x12220000
#define EXYNOS4_PA_SYSMMU_FIMC_ISP	0x12260000
#define EXYNOS4_PA_SYSMMU_FIMC_DRC	0x12270000
#define EXYNOS4_PA_SYSMMU_FIMC_FD	0x122A0000
#define EXYNOS4_PA_SYSMMU_ISPCPU	0x122B0000
#define EXYNOS4_PA_SYSMMU_FIMC_LITE0	0x123B0000
#define EXYNOS4_PA_SYSMMU_FIMC_LITE1	0x123C0000
#define EXYNOS4_PA_SYSMMU_PCIe		0x12620000
#define EXYNOS4_PA_SYSMMU_G2D		0x12A20000
#define EXYNOS4_PA_SYSMMU_ROTATOR	0x12A30000
#define EXYNOS4_PA_SYSMMU_MDMA2		0x12A40000
#define EXYNOS4_PA_SYSMMU_TV		0x12E20000
#define EXYNOS4_PA_SYSMMU_MFC_L		0x13620000
#define EXYNOS4_PA_SYSMMU_MFC_R		0x13630000

#define EXYNOS5_PA_SYSMMU_MDMA1		0x10A40000
#define EXYNOS5_PA_SYSMMU_SSS		0x10A50000
#define EXYNOS5_PA_SYSMMU_2D		0x10A60000
#define EXYNOS5_PA_SYSMMU_MFC_L		0x11200000
#define EXYNOS5_PA_SYSMMU_MFC_R		0x11210000
#define EXYNOS5_PA_SYSMMU_ROTATOR	0x11D40000
#define EXYNOS5_PA_SYSMMU_MDMA2		0x11D50000
#define EXYNOS5_PA_SYSMMU_JPEG		0x11F20000
#define EXYNOS5_PA_SYSMMU_IOP		0x12360000
#define EXYNOS5_PA_SYSMMU_RTIC		0x12370000
#define EXYNOS5_PA_SYSMMU_GPS		0x12630000
#define EXYNOS5_PA_SYSMMU_ISP		0x13260000
#define EXYNOS5_PA_SYSMMU_DRC		0x12370000
#define EXYNOS5_PA_SYSMMU_SCALERC	0x13280000
#define EXYNOS5_PA_SYSMMU_SCALERP	0x13290000
#define EXYNOS5_PA_SYSMMU_FD		0x132A0000
#define EXYNOS5_PA_SYSMMU_ISPCPU	0x132B0000
#define EXYNOS5_PA_SYSMMU_ODC		0x132C0000
#define EXYNOS5_PA_SYSMMU_DIS0		0x132D0000
#define EXYNOS5_PA_SYSMMU_DIS1		0x132E0000
#define EXYNOS5_PA_SYSMMU_3DNR		0x132F0000
#define EXYNOS5_PA_SYSMMU_LITE0		0x13C40000
#define EXYNOS5_PA_SYSMMU_LITE1		0x13C50000
#define EXYNOS5_PA_SYSMMU_GSC0		0x13E80000
#define EXYNOS5_PA_SYSMMU_GSC1		0x13E90000
#define EXYNOS5_PA_SYSMMU_GSC2		0x13EA0000
#define EXYNOS5_PA_SYSMMU_GSC3		0x13EB0000
#define EXYNOS5_PA_SYSMMU_FIMD1		0x14640000
#define EXYNOS5_PA_SYSMMU_TV		0x14650000

#define EXYNOS4_PA_SPI0			0x13920000
#define EXYNOS4_PA_SPI1			0x13930000
#define EXYNOS4_PA_SPI2			0x13940000
#define EXYNOS5_PA_SPI0			0x12D20000
#define EXYNOS5_PA_SPI1			0x12D30000
#define EXYNOS5_PA_SPI2			0x12D40000

#define EXYNOS4_PA_GPIO1		0x11400000
#define EXYNOS4_PA_GPIO2		0x11000000
#define EXYNOS4_PA_GPIO3		0x03860000
#define EXYNOS5_PA_GPIO1		0x11400000
#define EXYNOS5_PA_GPIO2		0x13400000
#define EXYNOS5_PA_GPIO3		0x10D10000
#define EXYNOS5_PA_GPIO4		0x03860000

#define EXYNOS4_PA_MIPI_CSIS0		0x11880000
#define EXYNOS4_PA_MIPI_CSIS1		0x11890000

#define EXYNOS4_PA_FIMD0		0x11C00000

#define EXYNOS4_PA_HSMMC(x)		(0x12510000 + ((x) * 0x10000))
#define EXYNOS4_PA_DWMCI		0x12550000

#define EXYNOS4_PA_HSOTG		0x12480000
#define EXYNOS4_PA_USB_HSPHY		0x125B0000

#define EXYNOS4_PA_SATA			0x12560000
#define EXYNOS4_PA_SATAPHY		0x125D0000
#define EXYNOS4_PA_SATAPHY_CTRL		0x126B0000

#define EXYNOS4_PA_SROMC		0x12570000
#define EXYNOS5_PA_SROMC		0x12250000

#define EXYNOS4_PA_EHCI			0x12580000
#define EXYNOS4_PA_OHCI			0x12590000
#define EXYNOS4_PA_HSPHY		0x125B0000
#define EXYNOS4_PA_MFC			0x13400000

#define EXYNOS4_PA_UART			0x13800000
#define EXYNOS5_PA_UART			0x12C00000

#define EXYNOS4_PA_VP			0x12C00000
#define EXYNOS4_PA_MIXER		0x12C10000
#define EXYNOS4_PA_SDO			0x12C20000
#define EXYNOS4_PA_HDMI			0x12D00000
#define EXYNOS4_PA_IIC_HDMIPHY		0x138E0000

#define EXYNOS4_PA_IIC(x)		(0x13860000 + ((x) * 0x10000))
#define EXYNOS5_PA_IIC(x)		(0x12C60000 + ((x) * 0x10000))

#define EXYNOS4_PA_ADC			0x13910000
#define EXYNOS4_PA_ADC1			0x13911000

#define EXYNOS4_PA_AC97			0x139A0000

#define EXYNOS4_PA_SPDIF		0x139B0000

#define EXYNOS4_PA_TIMER		0x139D0000
#define EXYNOS5_PA_TIMER		0x12DD0000

#define EXYNOS4_PA_SDRAM		0x40000000
#define EXYNOS5_PA_SDRAM		0x40000000

/* Compatibiltiy Defines */

#define S3C_PA_HSMMC0			EXYNOS4_PA_HSMMC(0)
#define S3C_PA_HSMMC1			EXYNOS4_PA_HSMMC(1)
#define S3C_PA_HSMMC2			EXYNOS4_PA_HSMMC(2)
#define S3C_PA_HSMMC3			EXYNOS4_PA_HSMMC(3)
#define S3C_PA_IIC			EXYNOS4_PA_IIC(0)
#define S3C_PA_IIC1			EXYNOS4_PA_IIC(1)
#define S3C_PA_IIC2			EXYNOS4_PA_IIC(2)
#define S3C_PA_IIC3			EXYNOS4_PA_IIC(3)
#define S3C_PA_IIC4			EXYNOS4_PA_IIC(4)
#define S3C_PA_IIC5			EXYNOS4_PA_IIC(5)
#define S3C_PA_IIC6			EXYNOS4_PA_IIC(6)
#define S3C_PA_IIC7			EXYNOS4_PA_IIC(7)
#define S3C_PA_RTC			EXYNOS4_PA_RTC
#define S3C_PA_WDT			EXYNOS4_PA_WATCHDOG
#define S3C_PA_SPI0			EXYNOS4_PA_SPI0
#define S3C_PA_SPI1			EXYNOS4_PA_SPI1
#define S3C_PA_SPI2			EXYNOS4_PA_SPI2
#define S3C_PA_USB_HSOTG		EXYNOS4_PA_HSOTG

#define S5P_PA_EHCI			EXYNOS4_PA_EHCI
#define S5P_PA_FIMC0			EXYNOS4_PA_FIMC0
#define S5P_PA_FIMC1			EXYNOS4_PA_FIMC1
#define S5P_PA_FIMC2			EXYNOS4_PA_FIMC2
#define S5P_PA_FIMC3			EXYNOS4_PA_FIMC3
#define S5P_PA_JPEG			EXYNOS4_PA_JPEG
#define S5P_PA_G2D			EXYNOS4_PA_G2D
#define S5P_PA_FIMD0			EXYNOS4_PA_FIMD0
#define S5P_PA_HDMI			EXYNOS4_PA_HDMI
#define S5P_PA_IIC_HDMIPHY		EXYNOS4_PA_IIC_HDMIPHY
#define S5P_PA_MFC			EXYNOS4_PA_MFC
#define S5P_PA_MIPI_CSIS0		EXYNOS4_PA_MIPI_CSIS0
#define S5P_PA_MIPI_CSIS1		EXYNOS4_PA_MIPI_CSIS1
#define S5P_PA_MIXER			EXYNOS4_PA_MIXER
#define S5P_PA_ONENAND			EXYNOS4_PA_ONENAND
#define S5P_PA_ONENAND_DMA		EXYNOS4_PA_ONENAND_DMA
#define S5P_PA_SDO			EXYNOS4_PA_SDO
#define S5P_PA_SDRAM			EXYNOS4_PA_SDRAM
#define S5P_PA_VP			EXYNOS4_PA_VP

#define SAMSUNG_PA_ADC			EXYNOS4_PA_ADC
#define SAMSUNG_PA_ADC1			EXYNOS4_PA_ADC1
#define SAMSUNG_PA_KEYPAD		EXYNOS4_PA_KEYPAD

/* Compatibility UART */

#define EXYNOS4_PA_UART0		0x13800000
#define EXYNOS4_PA_UART1		0x13810000
#define EXYNOS4_PA_UART2		0x13820000
#define EXYNOS4_PA_UART3		0x13830000
#define EXYNOS4_SZ_UART			SZ_256

#define EXYNOS5_PA_UART0		0x12C00000
#define EXYNOS5_PA_UART1		0x12C10000
#define EXYNOS5_PA_UART2		0x12C20000
#define EXYNOS5_PA_UART3		0x12C30000
#define EXYNOS5_SZ_UART			SZ_256

#define S3C_VA_UARTx(x)			(S3C_VA_UART + ((x) * S3C_UART_OFFSET))

#endif /* __ASM_ARCH_MAP_H */
