/*
 *  arch/arm/mach-versatile/include/mach/irqs.h
 *
 *  Copyright (C) 2003 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <mach/platform.h>

/* 
 *  IRQ interrupts definitions are the same as the INT definitions
 *  held within platform.h
 */
#define IRQ_VIC_START		0
#define IRQ_WDOGINT		(IRQ_VIC_START + INT_WDOGINT)
#define IRQ_SOFTINT		(IRQ_VIC_START + INT_SOFTINT)
#define IRQ_COMMRx		(IRQ_VIC_START + INT_COMMRx)
#define IRQ_COMMTx		(IRQ_VIC_START + INT_COMMTx)
#define IRQ_TIMERINT0_1		(IRQ_VIC_START + INT_TIMERINT0_1)
#define IRQ_TIMERINT2_3		(IRQ_VIC_START + INT_TIMERINT2_3)
#define IRQ_GPIOINT0		(IRQ_VIC_START + INT_GPIOINT0)
#define IRQ_GPIOINT1		(IRQ_VIC_START + INT_GPIOINT1)
#define IRQ_GPIOINT2		(IRQ_VIC_START + INT_GPIOINT2)
#define IRQ_GPIOINT3		(IRQ_VIC_START + INT_GPIOINT3)
#define IRQ_RTCINT		(IRQ_VIC_START + INT_RTCINT)
#define IRQ_SSPINT		(IRQ_VIC_START + INT_SSPINT)
#define IRQ_UARTINT0		(IRQ_VIC_START + INT_UARTINT0)
#define IRQ_UARTINT1		(IRQ_VIC_START + INT_UARTINT1)
#define IRQ_UARTINT2		(IRQ_VIC_START + INT_UARTINT2)
#define IRQ_SCIINT		(IRQ_VIC_START + INT_SCIINT)
#define IRQ_CLCDINT		(IRQ_VIC_START + INT_CLCDINT)
#define IRQ_DMAINT		(IRQ_VIC_START + INT_DMAINT)
#define IRQ_PWRFAILINT 		(IRQ_VIC_START + INT_PWRFAILINT)
#define IRQ_MBXINT		(IRQ_VIC_START + INT_MBXINT)
#define IRQ_GNDINT		(IRQ_VIC_START + INT_GNDINT)
#define IRQ_VICSOURCE21		(IRQ_VIC_START + INT_VICSOURCE21)
#define IRQ_VICSOURCE22		(IRQ_VIC_START + INT_VICSOURCE22)
#define IRQ_VICSOURCE23		(IRQ_VIC_START + INT_VICSOURCE23)
#define IRQ_VICSOURCE24		(IRQ_VIC_START + INT_VICSOURCE24)
#define IRQ_VICSOURCE25		(IRQ_VIC_START + INT_VICSOURCE25)
#define IRQ_VICSOURCE26		(IRQ_VIC_START + INT_VICSOURCE26)
#define IRQ_VICSOURCE27		(IRQ_VIC_START + INT_VICSOURCE27)
#define IRQ_VICSOURCE28		(IRQ_VIC_START + INT_VICSOURCE28)
#define IRQ_VICSOURCE29		(IRQ_VIC_START + INT_VICSOURCE29)
#define IRQ_VICSOURCE30		(IRQ_VIC_START + INT_VICSOURCE30)
#define IRQ_VICSOURCE31		(IRQ_VIC_START + INT_VICSOURCE31)
#define IRQ_VIC_END		(IRQ_VIC_START + 31)

/* 
 *  FIQ interrupts definitions are the same as the INT definitions.
 */
#define FIQ_WDOGINT		INT_WDOGINT
#define FIQ_SOFTINT		INT_SOFTINT
#define FIQ_COMMRx		INT_COMMRx
#define FIQ_COMMTx		INT_COMMTx
#define FIQ_TIMERINT0_1		INT_TIMERINT0_1
#define FIQ_TIMERINT2_3		INT_TIMERINT2_3
#define FIQ_GPIOINT0		INT_GPIOINT0
#define FIQ_GPIOINT1		INT_GPIOINT1
#define FIQ_GPIOINT2		INT_GPIOINT2
#define FIQ_GPIOINT3		INT_GPIOINT3
#define FIQ_RTCINT		INT_RTCINT
#define FIQ_SSPINT		INT_SSPINT
#define FIQ_UARTINT0		INT_UARTINT0
#define FIQ_UARTINT1		INT_UARTINT1
#define FIQ_UARTINT2		INT_UARTINT2
#define FIQ_SCIINT		INT_SCIINT
#define FIQ_CLCDINT		INT_CLCDINT
#define FIQ_DMAINT		INT_DMAINT
#define FIQ_PWRFAILINT 		INT_PWRFAILINT
#define FIQ_MBXINT		INT_MBXINT
#define FIQ_GNDINT		INT_GNDINT
#define FIQ_VICSOURCE21		INT_VICSOURCE21
#define FIQ_VICSOURCE22		INT_VICSOURCE22
#define FIQ_VICSOURCE23		INT_VICSOURCE23
#define FIQ_VICSOURCE24		INT_VICSOURCE24
#define FIQ_VICSOURCE25		INT_VICSOURCE25
#define FIQ_VICSOURCE26		INT_VICSOURCE26
#define FIQ_VICSOURCE27		INT_VICSOURCE27
#define FIQ_VICSOURCE28		INT_VICSOURCE28
#define FIQ_VICSOURCE29		INT_VICSOURCE29
#define FIQ_VICSOURCE30		INT_VICSOURCE30
#define FIQ_VICSOURCE31		INT_VICSOURCE31


/*
 * Secondary interrupt controller
 */
#define IRQ_SIC_START		32
#define IRQ_SIC_MMCI0B 		(IRQ_SIC_START + SIC_INT_MMCI0B)
#define IRQ_SIC_MMCI1B 		(IRQ_SIC_START + SIC_INT_MMCI1B)
#define IRQ_SIC_KMI0		(IRQ_SIC_START + SIC_INT_KMI0)
#define IRQ_SIC_KMI1		(IRQ_SIC_START + SIC_INT_KMI1)
#define IRQ_SIC_SCI3		(IRQ_SIC_START + SIC_INT_SCI3)
#define IRQ_SIC_UART3		(IRQ_SIC_START + SIC_INT_UART3)
#define IRQ_SIC_CLCD		(IRQ_SIC_START + SIC_INT_CLCD)
#define IRQ_SIC_TOUCH		(IRQ_SIC_START + SIC_INT_TOUCH)
#define IRQ_SIC_KEYPAD 		(IRQ_SIC_START + SIC_INT_KEYPAD)
#define IRQ_SIC_DoC		(IRQ_SIC_START + SIC_INT_DoC)
#define IRQ_SIC_MMCI0A 		(IRQ_SIC_START + SIC_INT_MMCI0A)
#define IRQ_SIC_MMCI1A 		(IRQ_SIC_START + SIC_INT_MMCI1A)
#define IRQ_SIC_AACI		(IRQ_SIC_START + SIC_INT_AACI)
#define IRQ_SIC_ETH		(IRQ_SIC_START + SIC_INT_ETH)
#define IRQ_SIC_USB		(IRQ_SIC_START + SIC_INT_USB)
#define IRQ_SIC_PCI0		(IRQ_SIC_START + SIC_INT_PCI0)
#define IRQ_SIC_PCI1		(IRQ_SIC_START + SIC_INT_PCI1)
#define IRQ_SIC_PCI2		(IRQ_SIC_START + SIC_INT_PCI2)
#define IRQ_SIC_PCI3		(IRQ_SIC_START + SIC_INT_PCI3)
#define IRQ_SIC_END		63

#define NR_IRQS			64
