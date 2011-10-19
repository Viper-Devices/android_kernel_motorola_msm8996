/*
 * Copyright (C) 2010 Renesas Solutions Corp.
 *
 * Kuninori Morimoto <morimoto.kuninori@renesas.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#ifndef __ASM_SH7372_H__
#define __ASM_SH7372_H__

#include <linux/sh_clk.h>
#include <linux/pm_domain.h>

/*
 * Pin Function Controller:
 *	GPIO_FN_xx - GPIO used to select pin function
 *	GPIO_PORTxx - GPIO mapped to real I/O pin on CPU
 */
enum {
	/* PORT */
	GPIO_PORT0, GPIO_PORT1, GPIO_PORT2, GPIO_PORT3, GPIO_PORT4,
	GPIO_PORT5, GPIO_PORT6, GPIO_PORT7, GPIO_PORT8, GPIO_PORT9,

	GPIO_PORT10, GPIO_PORT11, GPIO_PORT12, GPIO_PORT13, GPIO_PORT14,
	GPIO_PORT15, GPIO_PORT16, GPIO_PORT17, GPIO_PORT18, GPIO_PORT19,

	GPIO_PORT20, GPIO_PORT21, GPIO_PORT22, GPIO_PORT23, GPIO_PORT24,
	GPIO_PORT25, GPIO_PORT26, GPIO_PORT27, GPIO_PORT28, GPIO_PORT29,

	GPIO_PORT30, GPIO_PORT31, GPIO_PORT32, GPIO_PORT33, GPIO_PORT34,
	GPIO_PORT35, GPIO_PORT36, GPIO_PORT37, GPIO_PORT38, GPIO_PORT39,

	GPIO_PORT40, GPIO_PORT41, GPIO_PORT42, GPIO_PORT43, GPIO_PORT44,
	GPIO_PORT45, GPIO_PORT46, GPIO_PORT47, GPIO_PORT48, GPIO_PORT49,

	GPIO_PORT50, GPIO_PORT51, GPIO_PORT52, GPIO_PORT53, GPIO_PORT54,
	GPIO_PORT55, GPIO_PORT56, GPIO_PORT57, GPIO_PORT58, GPIO_PORT59,

	GPIO_PORT60, GPIO_PORT61, GPIO_PORT62, GPIO_PORT63, GPIO_PORT64,
	GPIO_PORT65, GPIO_PORT66, GPIO_PORT67, GPIO_PORT68, GPIO_PORT69,

	GPIO_PORT70, GPIO_PORT71, GPIO_PORT72, GPIO_PORT73, GPIO_PORT74,
	GPIO_PORT75, GPIO_PORT76, GPIO_PORT77, GPIO_PORT78, GPIO_PORT79,

	GPIO_PORT80, GPIO_PORT81, GPIO_PORT82, GPIO_PORT83, GPIO_PORT84,
	GPIO_PORT85, GPIO_PORT86, GPIO_PORT87, GPIO_PORT88, GPIO_PORT89,

	GPIO_PORT90, GPIO_PORT91, GPIO_PORT92, GPIO_PORT93, GPIO_PORT94,
	GPIO_PORT95, GPIO_PORT96, GPIO_PORT97, GPIO_PORT98, GPIO_PORT99,

	GPIO_PORT100, GPIO_PORT101, GPIO_PORT102, GPIO_PORT103, GPIO_PORT104,
	GPIO_PORT105, GPIO_PORT106, GPIO_PORT107, GPIO_PORT108, GPIO_PORT109,

	GPIO_PORT110, GPIO_PORT111, GPIO_PORT112, GPIO_PORT113, GPIO_PORT114,
	GPIO_PORT115, GPIO_PORT116, GPIO_PORT117, GPIO_PORT118, GPIO_PORT119,

	GPIO_PORT120, GPIO_PORT121, GPIO_PORT122, GPIO_PORT123, GPIO_PORT124,
	GPIO_PORT125, GPIO_PORT126, GPIO_PORT127, GPIO_PORT128, GPIO_PORT129,

	GPIO_PORT130, GPIO_PORT131, GPIO_PORT132, GPIO_PORT133, GPIO_PORT134,
	GPIO_PORT135, GPIO_PORT136, GPIO_PORT137, GPIO_PORT138, GPIO_PORT139,

	GPIO_PORT140, GPIO_PORT141, GPIO_PORT142, GPIO_PORT143, GPIO_PORT144,
	GPIO_PORT145, GPIO_PORT146, GPIO_PORT147, GPIO_PORT148, GPIO_PORT149,

	GPIO_PORT150, GPIO_PORT151, GPIO_PORT152, GPIO_PORT153, GPIO_PORT154,
	GPIO_PORT155, GPIO_PORT156, GPIO_PORT157, GPIO_PORT158, GPIO_PORT159,

	GPIO_PORT160, GPIO_PORT161, GPIO_PORT162, GPIO_PORT163, GPIO_PORT164,
	GPIO_PORT165, GPIO_PORT166, GPIO_PORT167, GPIO_PORT168, GPIO_PORT169,

	GPIO_PORT170, GPIO_PORT171, GPIO_PORT172, GPIO_PORT173, GPIO_PORT174,
	GPIO_PORT175, GPIO_PORT176, GPIO_PORT177, GPIO_PORT178, GPIO_PORT179,

	GPIO_PORT180, GPIO_PORT181, GPIO_PORT182, GPIO_PORT183, GPIO_PORT184,
	GPIO_PORT185, GPIO_PORT186, GPIO_PORT187, GPIO_PORT188, GPIO_PORT189,

	GPIO_PORT190,

	/* IRQ */
	GPIO_FN_IRQ0_6,		/* PORT   6 */
	GPIO_FN_IRQ0_162,	/* PORT 162 */
	GPIO_FN_IRQ1,		/* PORT  12 */
	GPIO_FN_IRQ2_4,		/* PORT   4 */
	GPIO_FN_IRQ2_5,		/* PORT   5 */
	GPIO_FN_IRQ3_8,		/* PORT   8 */
	GPIO_FN_IRQ3_16,	/* PORT  16 */
	GPIO_FN_IRQ4_17,	/* PORT  17 */
	GPIO_FN_IRQ4_163,	/* PORT 163 */
	GPIO_FN_IRQ5,		/* PORT  18 */
	GPIO_FN_IRQ6_39,	/* PORT  39 */
	GPIO_FN_IRQ6_164,	/* PORT 164 */
	GPIO_FN_IRQ7_40,	/* PORT  40 */
	GPIO_FN_IRQ7_167,	/* PORT 167 */
	GPIO_FN_IRQ8_41,	/* PORT  41 */
	GPIO_FN_IRQ8_168,	/* PORT 168 */
	GPIO_FN_IRQ9_42,	/* PORT  42 */
	GPIO_FN_IRQ9_169,	/* PORT 169 */
	GPIO_FN_IRQ10,		/* PORT  65 */
	GPIO_FN_IRQ11,		/* PORT  67 */
	GPIO_FN_IRQ12_80,	/* PORT  80 */
	GPIO_FN_IRQ12_137,	/* PORT 137 */
	GPIO_FN_IRQ13_81,	/* PORT  81 */
	GPIO_FN_IRQ13_145,	/* PORT 145 */
	GPIO_FN_IRQ14_82,	/* PORT  82 */
	GPIO_FN_IRQ14_146,	/* PORT 146 */
	GPIO_FN_IRQ15_83,	/* PORT  83 */
	GPIO_FN_IRQ15_147,	/* PORT 147 */
	GPIO_FN_IRQ16_84,	/* PORT  84 */
	GPIO_FN_IRQ16_170,	/* PORT 170 */
	GPIO_FN_IRQ17,		/* PORT  85 */
	GPIO_FN_IRQ18,		/* PORT  86 */
	GPIO_FN_IRQ19,		/* PORT  87 */
	GPIO_FN_IRQ20,		/* PORT  92 */
	GPIO_FN_IRQ21,		/* PORT  93 */
	GPIO_FN_IRQ22,		/* PORT  94 */
	GPIO_FN_IRQ23,		/* PORT  95 */
	GPIO_FN_IRQ24,		/* PORT 112 */
	GPIO_FN_IRQ25,		/* PORT 119 */
	GPIO_FN_IRQ26_121,	/* PORT 121 */
	GPIO_FN_IRQ26_172,	/* PORT 172 */
	GPIO_FN_IRQ27_122,	/* PORT 122 */
	GPIO_FN_IRQ27_180,	/* PORT 180 */
	GPIO_FN_IRQ28_123,	/* PORT 123 */
	GPIO_FN_IRQ28_181,	/* PORT 181 */
	GPIO_FN_IRQ29_129,	/* PORT 129 */
	GPIO_FN_IRQ29_182,	/* PORT 182 */
	GPIO_FN_IRQ30_130,	/* PORT 130 */
	GPIO_FN_IRQ30_183,	/* PORT 183 */
	GPIO_FN_IRQ31_138,	/* PORT 138 */
	GPIO_FN_IRQ31_184,	/* PORT 184 */

	/*
	 * MSIOF0	(PORT 36, 37, 38, 39
	 * 		      40, 41, 42, 43, 44, 45)
	 */
	GPIO_FN_MSIOF0_TSYNC,	GPIO_FN_MSIOF0_TSCK,
	GPIO_FN_MSIOF0_RXD,	GPIO_FN_MSIOF0_RSCK,
	GPIO_FN_MSIOF0_RSYNC,	GPIO_FN_MSIOF0_MCK0,
	GPIO_FN_MSIOF0_MCK1,	GPIO_FN_MSIOF0_SS1,
	GPIO_FN_MSIOF0_SS2,	GPIO_FN_MSIOF0_TXD,

	/*
	 * MSIOF1	(PORT 39, 40, 41, 42, 43, 44
	 * 		      84, 85, 86, 87, 88, 89, 90, 91, 92, 93)
	 */
	GPIO_FN_MSIOF1_TSCK_39,	GPIO_FN_MSIOF1_TSYNC_40,
	GPIO_FN_MSIOF1_TSCK_88,	GPIO_FN_MSIOF1_TSYNC_89,
	GPIO_FN_MSIOF1_TXD_41,	GPIO_FN_MSIOF1_RXD_42,
	GPIO_FN_MSIOF1_TXD_90,	GPIO_FN_MSIOF1_RXD_91,
	GPIO_FN_MSIOF1_SS1_43,	GPIO_FN_MSIOF1_SS2_44,
	GPIO_FN_MSIOF1_SS1_92,	GPIO_FN_MSIOF1_SS2_93,
	GPIO_FN_MSIOF1_RSCK,	GPIO_FN_MSIOF1_RSYNC,
	GPIO_FN_MSIOF1_MCK0,	GPIO_FN_MSIOF1_MCK1,

	/*
	 * MSIOF2	(PORT 134, 135, 136, 137, 138, 139
	 *		      148, 149, 150, 151)
	 */
	GPIO_FN_MSIOF2_RSCK,	GPIO_FN_MSIOF2_RSYNC,
	GPIO_FN_MSIOF2_MCK0,	GPIO_FN_MSIOF2_MCK1,
	GPIO_FN_MSIOF2_SS1,	GPIO_FN_MSIOF2_SS2,
	GPIO_FN_MSIOF2_TSYNC,	GPIO_FN_MSIOF2_TSCK,
	GPIO_FN_MSIOF2_RXD,	GPIO_FN_MSIOF2_TXD,

	/* MSIOF3	(PORT 76, 77, 78, 79, 80, 81, 82, 83) */
	GPIO_FN_BBIF1_RXD,	GPIO_FN_BBIF1_TSYNC,
	GPIO_FN_BBIF1_TSCK,	GPIO_FN_BBIF1_TXD,
	GPIO_FN_BBIF1_RSCK,	GPIO_FN_BBIF1_RSYNC,
	GPIO_FN_BBIF1_FLOW,	GPIO_FN_BB_RX_FLOW_N,

	/* MSIOF4	(PORT 0, 1, 2, 3) */
	GPIO_FN_BBIF2_TSCK1,	GPIO_FN_BBIF2_TSYNC1,
	GPIO_FN_BBIF2_TXD1,	GPIO_FN_BBIF2_RXD,

	/* FSI		(PORT 4, 5, 6, 7, 8, 9, 10, 11, 15) */
	GPIO_FN_FSIACK,		GPIO_FN_FSIBCK,
	GPIO_FN_FSIAILR,	GPIO_FN_FSIAIBT,
	GPIO_FN_FSIAISLD,	GPIO_FN_FSIAOMC,
	GPIO_FN_FSIAOLR,	GPIO_FN_FSIAOBT,
	GPIO_FN_FSIAOSLD,	GPIO_FN_FSIASPDIF_11,
	GPIO_FN_FSIASPDIF_15,

	/* FMSI		(PORT 12, 13, 14, 15, 16, 17, 18, 65) */
	GPIO_FN_FMSOCK,		GPIO_FN_FMSOOLR,
	GPIO_FN_FMSIOLR,	GPIO_FN_FMSOOBT,
	GPIO_FN_FMSIOBT,	GPIO_FN_FMSOSLD,
	GPIO_FN_FMSOILR,	GPIO_FN_FMSIILR,
	GPIO_FN_FMSOIBT,	GPIO_FN_FMSIIBT,
	GPIO_FN_FMSISLD,	GPIO_FN_FMSICK,

	/* SCIFA0	(PORT 152, 153, 156, 157, 158) */
	GPIO_FN_SCIFA0_TXD,	GPIO_FN_SCIFA0_RXD,
	GPIO_FN_SCIFA0_SCK,	GPIO_FN_SCIFA0_RTS,
	GPIO_FN_SCIFA0_CTS,

	/* SCIFA1	(PORT 154, 155, 159, 160, 161) */
	GPIO_FN_SCIFA1_TXD,	GPIO_FN_SCIFA1_RXD,
	GPIO_FN_SCIFA1_SCK,	GPIO_FN_SCIFA1_RTS,
	GPIO_FN_SCIFA1_CTS,

	/* SCIFA2	(PORT 94, 95, 96, 97, 98) */
	GPIO_FN_SCIFA2_CTS1,	GPIO_FN_SCIFA2_RTS1,
	GPIO_FN_SCIFA2_TXD1,	GPIO_FN_SCIFA2_RXD1,
	GPIO_FN_SCIFA2_SCK1,

	/* SCIFA3	(PORT 43, 44,
			     140, 141, 142, 143, 144) */
	GPIO_FN_SCIFA3_CTS_43,	GPIO_FN_SCIFA3_CTS_140,
	GPIO_FN_SCIFA3_RTS_44,	GPIO_FN_SCIFA3_RTS_141,
	GPIO_FN_SCIFA3_SCK,	GPIO_FN_SCIFA3_TXD,
	GPIO_FN_SCIFA3_RXD,

	/* SCIFA4	(PORT 5, 6) */
	GPIO_FN_SCIFA4_RXD,	GPIO_FN_SCIFA4_TXD,

	/* SCIFA5	(PORT 8, 12) */
	GPIO_FN_SCIFA5_RXD,	GPIO_FN_SCIFA5_TXD,

	/* SCIFB	(PORT 162, 163, 164, 165, 166) */
	GPIO_FN_SCIFB_SCK,	GPIO_FN_SCIFB_RTS,
	GPIO_FN_SCIFB_CTS,	GPIO_FN_SCIFB_TXD,
	GPIO_FN_SCIFB_RXD,

	/*
	 * CEU		(PORT 16, 17,
	 *		      100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
	 *		      110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
	 *		      120)
	 */
	GPIO_FN_VIO_HD,		GPIO_FN_VIO_CKO1,	GPIO_FN_VIO_CKO2,
	GPIO_FN_VIO_VD,		GPIO_FN_VIO_CLK,	GPIO_FN_VIO_FIELD,
	GPIO_FN_VIO_CKO,
	GPIO_FN_VIO_D0,		GPIO_FN_VIO_D1,		GPIO_FN_VIO_D2,
	GPIO_FN_VIO_D3,		GPIO_FN_VIO_D4,		GPIO_FN_VIO_D5,
	GPIO_FN_VIO_D6,		GPIO_FN_VIO_D7,		GPIO_FN_VIO_D8,
	GPIO_FN_VIO_D9,		GPIO_FN_VIO_D10,	GPIO_FN_VIO_D11,
	GPIO_FN_VIO_D12,	GPIO_FN_VIO_D13,	GPIO_FN_VIO_D14,
	GPIO_FN_VIO_D15,

	/* USB0		(PORT 113, 114, 115, 116, 117, 167) */
	GPIO_FN_IDIN_0,		GPIO_FN_EXTLP_0,
	GPIO_FN_OVCN2_0,	GPIO_FN_PWEN_0,
	GPIO_FN_OVCN_0,		GPIO_FN_VBUS0_0,

	/* USB1		(PORT 18, 113, 114, 115, 116, 117, 138, 162, 168) */
	GPIO_FN_IDIN_1_18,	GPIO_FN_IDIN_1_113,
	GPIO_FN_PWEN_1_115,	GPIO_FN_PWEN_1_138,
	GPIO_FN_OVCN_1_114,	GPIO_FN_OVCN_1_162,
	GPIO_FN_EXTLP_1,	GPIO_FN_OVCN2_1,
	GPIO_FN_VBUS0_1,

	/* GPIO		(PORT 41, 42, 43, 44) */
	GPIO_FN_GPI0,	GPIO_FN_GPI1,	GPIO_FN_GPO0,	GPIO_FN_GPO1,

	/*
	 * BSC		(PORT 19,
	 *		      20, 21, 22, 25, 26, 27, 28, 29,
	 *		      30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
	 *		      40, 41, 42, 43, 44, 45,
	 *		      62, 63, 64, 65, 66, 67,
	 *		      71, 72, 74, 75)
	 */
	GPIO_FN_BS,	GPIO_FN_WE1,
	GPIO_FN_CKO,	GPIO_FN_WAIT,	GPIO_FN_RDWR,

	GPIO_FN_A0,	GPIO_FN_A1,	GPIO_FN_A2,	GPIO_FN_A3,
	GPIO_FN_A6,	GPIO_FN_A7,	GPIO_FN_A8,	GPIO_FN_A9,
	GPIO_FN_A10,	GPIO_FN_A11,	GPIO_FN_A12,	GPIO_FN_A13,
	GPIO_FN_A14,	GPIO_FN_A15,	GPIO_FN_A16,	GPIO_FN_A17,
	GPIO_FN_A18,	GPIO_FN_A19,	GPIO_FN_A20,	GPIO_FN_A21,
	GPIO_FN_A22,	GPIO_FN_A23,	GPIO_FN_A24,	GPIO_FN_A25,
	GPIO_FN_A26,

	GPIO_FN_CS0,	GPIO_FN_CS2,	GPIO_FN_CS4,
	GPIO_FN_CS5A,	GPIO_FN_CS5B,	GPIO_FN_CS6A,

	/*
	 * BSC/FLCTL		(PORT 23, 24,
	 *			      46, 47, 48, 49,
	 *			      50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
	 *			      60, 61, 69, 70)
	 */
	GPIO_FN_RD_FSC,		GPIO_FN_WE0_FWE,
	GPIO_FN_A4_FOE,		GPIO_FN_A5_FCDE,
	GPIO_FN_D0_NAF0,	GPIO_FN_D1_NAF1,	GPIO_FN_D2_NAF2,
	GPIO_FN_D3_NAF3,	GPIO_FN_D4_NAF4,	GPIO_FN_D5_NAF5,
	GPIO_FN_D6_NAF6,	GPIO_FN_D7_NAF7,	GPIO_FN_D8_NAF8,
	GPIO_FN_D9_NAF9,	GPIO_FN_D10_NAF10,	GPIO_FN_D11_NAF11,
	GPIO_FN_D12_NAF12,	GPIO_FN_D13_NAF13,	GPIO_FN_D14_NAF14,
	GPIO_FN_D15_NAF15,

	/*
	 * MMCIF(1)		(PORT 84, 85, 86, 87, 88, 89,
	 *			      90, 91, 92, 99)
	 */
	GPIO_FN_MMCD0_0,	GPIO_FN_MMCD0_1,	GPIO_FN_MMCD0_2,
	GPIO_FN_MMCD0_3,	GPIO_FN_MMCD0_4,	GPIO_FN_MMCD0_5,
	GPIO_FN_MMCD0_6,	GPIO_FN_MMCD0_7,
	GPIO_FN_MMCCMD0,	GPIO_FN_MMCCLK0,

	/* MMCIF(2)		(PORT 54, 55, 56, 57, 58, 59, 60, 61, 66, 67) */
	GPIO_FN_MMCD1_0,	GPIO_FN_MMCD1_1,	GPIO_FN_MMCD1_2,
	GPIO_FN_MMCD1_3,	GPIO_FN_MMCD1_4,	GPIO_FN_MMCD1_5,
	GPIO_FN_MMCD1_6,	GPIO_FN_MMCD1_7,
	GPIO_FN_MMCCLK1,	GPIO_FN_MMCCMD1,

	/* SPU2		(PORT 65) */
	GPIO_FN_VINT_I,

	/* FLCTL	(PORT 66, 68, 73) */
	GPIO_FN_FCE1,	GPIO_FN_FCE0,	GPIO_FN_FRB,

	/* HSI		(PORT 76, 77, 78, 79, 80, 81, 82, 83) */
	GPIO_FN_GP_RX_FLAG,	GPIO_FN_GP_RX_DATA,	GPIO_FN_GP_TX_READY,
	GPIO_FN_GP_RX_WAKE,	GPIO_FN_MP_TX_FLAG,	GPIO_FN_MP_TX_DATA,
	GPIO_FN_MP_RX_READY,	GPIO_FN_MP_TX_WAKE,

	/*
	 * MFI		(PORT 76, 77, 78, 79,
	 *		      80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
	 *		      90, 91, 92, 93, 94, 95, 96, 97, 98, 99)
	 */
	GPIO_FN_MFIv6,	/* see MSEL4CR 6 */
	GPIO_FN_MFIv4,	/* see MSEL4CR 6 */

	GPIO_FN_MEMC_CS0,		GPIO_FN_MEMC_BUSCLK_MEMC_A0,
	GPIO_FN_MEMC_CS1_MEMC_A1,	GPIO_FN_MEMC_ADV_MEMC_DREQ0,
	GPIO_FN_MEMC_WAIT_MEMC_DREQ1,	GPIO_FN_MEMC_NOE,
	GPIO_FN_MEMC_NWE,		GPIO_FN_MEMC_INT,

	GPIO_FN_MEMC_AD0,	GPIO_FN_MEMC_AD1,	GPIO_FN_MEMC_AD2,
	GPIO_FN_MEMC_AD3,	GPIO_FN_MEMC_AD4,	GPIO_FN_MEMC_AD5,
	GPIO_FN_MEMC_AD6,	GPIO_FN_MEMC_AD7,	GPIO_FN_MEMC_AD8,
	GPIO_FN_MEMC_AD9,	GPIO_FN_MEMC_AD10,	GPIO_FN_MEMC_AD11,
	GPIO_FN_MEMC_AD12,	GPIO_FN_MEMC_AD13,	GPIO_FN_MEMC_AD14,
	GPIO_FN_MEMC_AD15,

	/* SIM		(PORT 94, 95, 98) */
	GPIO_FN_SIM_RST,	GPIO_FN_SIM_CLK,	GPIO_FN_SIM_D,

	/* TPU		(PORT 93, 99, 112, 160, 161) */
	GPIO_FN_TPU0TO0,	GPIO_FN_TPU0TO1,
	GPIO_FN_TPU0TO2_93,	GPIO_FN_TPU0TO2_99,
	GPIO_FN_TPU0TO3,

	/* I2C2		(PORT 110, 111) */
	GPIO_FN_I2C_SCL2,	GPIO_FN_I2C_SDA2,

	/* I2C3(1)	(PORT 114, 115) */
	GPIO_FN_I2C_SCL3,	GPIO_FN_I2C_SDA3,

	/* I2C3(2)	(PORT 137, 145) */
	GPIO_FN_I2C_SCL3S,	GPIO_FN_I2C_SDA3S,

	/* I2C4(2)	(PORT 116, 117) */
	GPIO_FN_I2C_SCL4,	GPIO_FN_I2C_SDA4,

	/* I2C4(2)	(PORT 146, 147) */
	GPIO_FN_I2C_SCL4S,	GPIO_FN_I2C_SDA4S,

	/*
	 * KEYSC	(PORT 121, 122, 123, 124, 125, 126, 127, 128, 129,
	 *		      130, 131, 132, 133, 134, 135, 136)
	 */
	GPIO_FN_KEYOUT0,	GPIO_FN_KEYIN0_121,	GPIO_FN_KEYIN0_136,
	GPIO_FN_KEYOUT1,	GPIO_FN_KEYIN1_122,	GPIO_FN_KEYIN1_135,
	GPIO_FN_KEYOUT2,	GPIO_FN_KEYIN2_123,	GPIO_FN_KEYIN2_134,
	GPIO_FN_KEYOUT3,	GPIO_FN_KEYIN3_124,	GPIO_FN_KEYIN3_133,
	GPIO_FN_KEYOUT4,	GPIO_FN_KEYIN4,
	GPIO_FN_KEYOUT5,	GPIO_FN_KEYIN5,
	GPIO_FN_KEYOUT6,	GPIO_FN_KEYIN6,
	GPIO_FN_KEYOUT7,	GPIO_FN_KEYIN7,

	/*
	 * LCDC		(PORT      121, 122, 123, 124, 125, 126, 127, 128, 129,
	 *		      130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
	 *		      140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
	 *		      150, 151)
	 */
	GPIO_FN_LCDC0_SELECT, /* LCDC 0 */
	GPIO_FN_LCDC1_SELECT, /* LCDC 1 */
	GPIO_FN_LCDHSYN,	GPIO_FN_LCDCS,	GPIO_FN_LCDVSYN,
	GPIO_FN_LCDDCK,		GPIO_FN_LCDWR,	GPIO_FN_LCDRD,
	GPIO_FN_LCDDISP,	GPIO_FN_LCDRS,	GPIO_FN_LCDLCLK,
	GPIO_FN_LCDDON,

	GPIO_FN_LCDD0,	GPIO_FN_LCDD1,	GPIO_FN_LCDD2,	GPIO_FN_LCDD3,
	GPIO_FN_LCDD4,	GPIO_FN_LCDD5,	GPIO_FN_LCDD6,	GPIO_FN_LCDD7,
	GPIO_FN_LCDD8,	GPIO_FN_LCDD9,	GPIO_FN_LCDD10,	GPIO_FN_LCDD11,
	GPIO_FN_LCDD12,	GPIO_FN_LCDD13,	GPIO_FN_LCDD14,	GPIO_FN_LCDD15,
	GPIO_FN_LCDD16,	GPIO_FN_LCDD17,	GPIO_FN_LCDD18,	GPIO_FN_LCDD19,
	GPIO_FN_LCDD20,	GPIO_FN_LCDD21,	GPIO_FN_LCDD22,	GPIO_FN_LCDD23,

	/* IRDA		(PORT 139, 140, 141, 142) */
	GPIO_FN_IRDA_OUT,	GPIO_FN_IRDA_IN,	GPIO_FN_IRDA_FIRSEL,
	GPIO_FN_IROUT_139,	GPIO_FN_IROUT_140,

	/* TSIF1	(PORT 156, 157, 158, 159) */
	GPIO_FN_TS0_1SELECT, /* TSIF0 - 1 select */
	GPIO_FN_TS0_2SELECT, /* TSIF0 - 2 select */
	GPIO_FN_TS1_1SELECT, /* TSIF1 - 1 select */
	GPIO_FN_TS1_2SELECT, /* TSIF1 - 2 select */

	GPIO_FN_TS_SPSYNC1,	GPIO_FN_TS_SDAT1,
	GPIO_FN_TS_SDEN1,	GPIO_FN_TS_SCK1,

	/* TSIF2	(PORT 137, 145, 146, 147) */
	GPIO_FN_TS_SPSYNC2,	GPIO_FN_TS_SDAT2,
	GPIO_FN_TS_SDEN2,	GPIO_FN_TS_SCK2,

	/* HDMI		(PORT 169, 170) */
	GPIO_FN_HDMI_HPD,	GPIO_FN_HDMI_CEC,

	/* SDHI0	(PORT 171, 172, 173, 174, 175, 176, 177, 178) */
	GPIO_FN_SDHICLK0,	GPIO_FN_SDHICD0,
	GPIO_FN_SDHICMD0,	GPIO_FN_SDHIWP0,
	GPIO_FN_SDHID0_0,	GPIO_FN_SDHID0_1,
	GPIO_FN_SDHID0_2,	GPIO_FN_SDHID0_3,

	/* SDHI1	(PORT 179, 180, 181, 182, 183, 184) */
	GPIO_FN_SDHICLK1,	GPIO_FN_SDHICMD1,	GPIO_FN_SDHID1_0,
	GPIO_FN_SDHID1_1,	GPIO_FN_SDHID1_2,	GPIO_FN_SDHID1_3,

	/* SDHI2	(PORT 185, 186, 187, 188, 189, 190) */
	GPIO_FN_SDHICLK2,	GPIO_FN_SDHICMD2,	GPIO_FN_SDHID2_0,
	GPIO_FN_SDHID2_1,	GPIO_FN_SDHID2_2,	GPIO_FN_SDHID2_3,

	/* SDENC	see MSEL4CR 19 */
	GPIO_FN_SDENC_CPG,
	GPIO_FN_SDENC_DV_CLKI,
};

/* DMA slave IDs */
enum {
	SHDMA_SLAVE_INVALID,
	SHDMA_SLAVE_SCIF0_TX,
	SHDMA_SLAVE_SCIF0_RX,
	SHDMA_SLAVE_SCIF1_TX,
	SHDMA_SLAVE_SCIF1_RX,
	SHDMA_SLAVE_SCIF2_TX,
	SHDMA_SLAVE_SCIF2_RX,
	SHDMA_SLAVE_SCIF3_TX,
	SHDMA_SLAVE_SCIF3_RX,
	SHDMA_SLAVE_SCIF4_TX,
	SHDMA_SLAVE_SCIF4_RX,
	SHDMA_SLAVE_SCIF5_TX,
	SHDMA_SLAVE_SCIF5_RX,
	SHDMA_SLAVE_SCIF6_TX,
	SHDMA_SLAVE_SCIF6_RX,
	SHDMA_SLAVE_SDHI0_RX,
	SHDMA_SLAVE_SDHI0_TX,
	SHDMA_SLAVE_SDHI1_RX,
	SHDMA_SLAVE_SDHI1_TX,
	SHDMA_SLAVE_SDHI2_RX,
	SHDMA_SLAVE_SDHI2_TX,
	SHDMA_SLAVE_MMCIF_RX,
	SHDMA_SLAVE_MMCIF_TX,
	SHDMA_SLAVE_USB0_TX,
	SHDMA_SLAVE_USB0_RX,
	SHDMA_SLAVE_USB1_TX,
	SHDMA_SLAVE_USB1_RX,
};

extern struct clk sh7372_extal1_clk;
extern struct clk sh7372_extal2_clk;
extern struct clk sh7372_dv_clki_clk;
extern struct clk sh7372_dv_clki_div2_clk;
extern struct clk sh7372_pllc2_clk;
extern struct clk sh7372_fsiack_clk;
extern struct clk sh7372_fsibck_clk;
extern struct clk sh7372_fsidiva_clk;
extern struct clk sh7372_fsidivb_clk;

struct platform_device;

struct sh7372_pm_domain {
	struct generic_pm_domain genpd;
	struct dev_power_governor *gov;
	unsigned int bit_shift;
	bool no_debug;
};

static inline struct sh7372_pm_domain *to_sh7372_pd(struct generic_pm_domain *d)
{
	return container_of(d, struct sh7372_pm_domain, genpd);
}

#ifdef CONFIG_PM
extern struct sh7372_pm_domain sh7372_a4lc;
extern struct sh7372_pm_domain sh7372_a4mp;
extern struct sh7372_pm_domain sh7372_d4;
extern struct sh7372_pm_domain sh7372_a3rv;
extern struct sh7372_pm_domain sh7372_a3ri;
extern struct sh7372_pm_domain sh7372_a3sp;
extern struct sh7372_pm_domain sh7372_a3sg;

extern void sh7372_init_pm_domain(struct sh7372_pm_domain *sh7372_pd);
extern void sh7372_add_device_to_domain(struct sh7372_pm_domain *sh7372_pd,
					struct platform_device *pdev);
extern void sh7372_pm_add_subdomain(struct sh7372_pm_domain *sh7372_pd,
				    struct sh7372_pm_domain *sh7372_sd);
#else
#define sh7372_init_pm_domain(pd) do { } while(0)
#define sh7372_add_device_to_domain(pd, pdev) do { } while(0)
#define sh7372_pm_add_subdomain(pd, sd) do { } while(0)
#endif /* CONFIG_PM */

#endif /* __ASM_SH7372_H__ */
