/*
 * r8a7779 processor support - INTC hardware block
 *
 * Copyright (C) 2011  Renesas Solutions Corp.
 * Copyright (C) 2011  Magnus Damm
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/irqchip/arm-gic.h>
#include <mach/common.h>
#include <linux/irqchip.h>
#include <mach/intc.h>
#include <mach/r8a7779.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#define INT2SMSKCR0 IOMEM(0xfe7822a0)
#define INT2SMSKCR1 IOMEM(0xfe7822a4)
#define INT2SMSKCR2 IOMEM(0xfe7822a8)
#define INT2SMSKCR3 IOMEM(0xfe7822ac)
#define INT2SMSKCR4 IOMEM(0xfe7822b0)

#define INT2NTSR0 IOMEM(0xfe700060)
#define INT2NTSR1 IOMEM(0xfe700064)

static int r8a7779_set_wake(struct irq_data *data, unsigned int on)
{
	return 0; /* always allow wakeup */
}

static void __init r8a7779_init_irq_common(void)
{
	gic_arch_extn.irq_set_wake = r8a7779_set_wake;

	/* route all interrupts to ARM */
	__raw_writel(0xffffffff, INT2NTSR0);
	__raw_writel(0x3fffffff, INT2NTSR1);

	/* unmask all known interrupts in INTCS2 */
	__raw_writel(0xfffffff0, INT2SMSKCR0);
	__raw_writel(0xfff7ffff, INT2SMSKCR1);
	__raw_writel(0xfffbffdf, INT2SMSKCR2);
	__raw_writel(0xbffffffc, INT2SMSKCR3);
	__raw_writel(0x003fee3f, INT2SMSKCR4);
}

void __init r8a7779_init_irq(void)
{
	void __iomem *gic_dist_base = IOMEM(0xf0001000);
	void __iomem *gic_cpu_base = IOMEM(0xf0000100);

	/* use GIC to handle interrupts */
	gic_init(0, 29, gic_dist_base, gic_cpu_base);

	r8a7779_init_irq_common();
}

#ifdef CONFIG_OF
void __init r8a7779_init_irq_dt(void)
{
	irqchip_init();
	r8a7779_init_irq_common();
}
#endif
